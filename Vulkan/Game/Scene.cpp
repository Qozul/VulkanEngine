#include "Scene.h"
#include "../Assets/Entity.h"
#include "../Assets/Transform.h"
#include "../SystemMasters.h"
#include "../Graphics/GraphicsMaster.h"
#include "ParticleSystem.h"
#include "../Graphics/StorageBuffer.h"
#include "../Graphics/Descriptor.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/LogicalCamera.h"
#include "../Graphics/MeshLoader.h"

using namespace QZL;
using namespace Graphics;

Scene::Scene(const SystemMasters* masters)
	: masters_(masters)
{
	rootNode_ = new SceneHeirarchyNode();
	rootNode_->parentNode = nullptr;
	rootNode_->entity = nullptr;
}

Scene::~Scene()
{
	deleteHeirarchyRecursively(rootNode_);
	SAFE_DELETE(graphicsInfo_.mvpBuffer);
	SAFE_DELETE(graphicsInfo_.paramsBuffer);
	SAFE_DELETE(graphicsInfo_.materialBuffer);
}

std::vector<VkDrawIndexedIndirectCommand>* Scene::update(glm::mat4& viewProjection, float dt, const uint32_t& frameIdx, LogicalCamera& mainCamera)
{
	for (auto& cmdList : graphicsCommandLists_) {
		cmdList.clear();
	}
	for (auto& distances : graphicsWriteInfo_.distances) {
		distances.clear(); // TODO use these for sorting the commands and data
	}
	std::memset(graphicsWriteInfo_.offsets, 0, (size_t)Graphics::RendererTypes::kNone * sizeof(VkDeviceSize));
	std::memset(graphicsWriteInfo_.graphicsMVPData.data(), 0, graphicsInfo_.mvpRange);
	std::memset(graphicsWriteInfo_.graphicsParamsData.data(), 0, graphicsInfo_.paramsRange);
	std::memset(graphicsWriteInfo_.graphicsMaterialData.data(), 0, graphicsInfo_.materialRange);

	for (size_t i = 0; i < rootNode_->childNodes.size(); ++i) {
		updateRecursively(rootNode_->childNodes[i], viewProjection, glm::mat4(), dt, frameIdx, mainCamera);
	}

	// Write frame's data to the gpu buffers
	graphicsWriteInfo_.mvpPtr = (char*)graphicsInfo_.mvpBuffer->bindRange();
	std::memcpy(graphicsWriteInfo_.mvpPtr + frameIdx * graphicsInfo_.mvpRange, graphicsWriteInfo_.graphicsMVPData.data(), graphicsInfo_.mvpRange);
	graphicsWriteInfo_.paramsPtr = (char*)graphicsInfo_.paramsBuffer->bindRange();
	graphicsWriteInfo_.mvpPtr = (char*)graphicsInfo_.mvpBuffer->unbindRange();
	std::memcpy(graphicsWriteInfo_.paramsPtr + frameIdx * graphicsInfo_.paramsRange, graphicsWriteInfo_.graphicsParamsData.data(), graphicsInfo_.paramsRange);
	graphicsWriteInfo_.paramsPtr = (char*)graphicsInfo_.paramsBuffer->unbindRange();
	graphicsWriteInfo_.materialPtr = (char*)graphicsInfo_.materialBuffer->bindRange();
	std::memcpy(graphicsWriteInfo_.materialPtr + frameIdx * graphicsInfo_.materialRange, graphicsWriteInfo_.graphicsMaterialData.data(), graphicsInfo_.materialRange);
	graphicsWriteInfo_.materialPtr = (char*)graphicsInfo_.materialBuffer->unbindRange();

	return graphicsCommandLists_;
}

void Scene::start()
{
	for (size_t i = 0; i < (size_t)RendererTypes::kNone; ++i) {
		graphicsInfo_.shadowCastingEBOs[i] = kRendererTypeFlags[i] & RendererFlags::CASTS_SHADOWS ? masters_->graphicsMaster->getDynamicBuffer((RendererTypes)i) : nullptr;
	}
	for (size_t i = 0; i < rootNode_->childNodes.size(); ++i) {
		startRecursively(rootNode_->childNodes[i]);
	}
}

SceneHeirarchyNode* Scene::addEntity(Entity* entity, Entity* parent, SceneHeirarchyNode* hintNode)
{
	ASSERT(entity != nullptr);
	ASSERT_DEBUG(findEntityNode(entity) == nullptr); // This check could be expensive, so only test in debug mode

	SceneHeirarchyNode* parentNode = parent == nullptr ? rootNode_ :
		hintNode != nullptr ? hintNode : findEntityNode(parent);
	ASSERT(parentNode != nullptr);
	SceneHeirarchyNode* childNode = new SceneHeirarchyNode();
	childNode->parentNode = parentNode;
	childNode->entity = entity;
	parentNode->childNodes.push_back(childNode);
	childNode->entity->setSceneNode(childNode);

	return childNode;
}

void Scene::removeEntity(Entity* entity, bool reparent, SceneHeirarchyNode* hintNode)
{
	ASSERT(entity != nullptr);
	SceneHeirarchyNode* node = hintNode != nullptr ? hintNode : findEntityNode(entity);
	if (node != nullptr) {
		if (!reparent) {
			deleteHeirarchyRecursively(node);
		}
		else {
			auto parentChildren = node->parentNode->childNodes;
			for (size_t i = 0; i < node->childNodes.size(); ++i) {
				parentChildren.push_back(node->childNodes[i]);
			}
			parentChildren.erase(std::find(parentChildren.begin(), parentChildren.end(), node));
			node->entity->setSceneNode(nullptr);
			SAFE_DELETE(node);
		}
	}
}

SceneHeirarchyNode* Scene::findEntityNode(Entity* entity)
{
	// Beginning from the root node, check child nodes in order as a tree
	if (entity == nullptr) {
		return rootNode_;
	}
	for (size_t i = 0; i < rootNode_->childNodes.size(); ++i) {
		auto node = findEntityNodeRecursively(entity, rootNode_->childNodes[i]);
		if (node != nullptr) {
			return node;
		}
	}
	return nullptr;
}

void Scene::findDescriptorRequirements(std::unordered_map<Graphics::RendererTypes, uint32_t>& instancesCount)
{
	for (size_t i = 0; i < rootNode_->childNodes.size(); ++i) {
		findDescriptorRequirementsRecursively(instancesCount, rootNode_->childNodes[i]);
	}
}

struct Data {
	int id;
	VkDeviceSize size;
	size_t count;
};

struct IN {
	VkDeviceSize sizeMultiplier; // i.e. frameImageCount
	VkDeviceSize deviceOffsetAlignment;
	uint32_t binding;
	std::string name;
	VkShaderStageFlags stages;
	std::vector<Data> data;
};

struct OUT_YEAH {
	DescriptorBuffer* buffer;
	VkDeviceSize dynamicOffset;
	std::vector<std::pair<size_t, VkDeviceSize>> dataOffsets;
};

bool compareFunc(Data& a, Data& b)
{
	return a.size > b.size;
}

OUT_YEAH coolDescriptor(IN info, const LogicDevice* logicDevice) {
	ASSERT_DEBUG(info.data.size() > 0);
	std::sort(info.data.begin(), info.data.end(), compareFunc);
	OUT_YEAH result;
	result.dataOffsets.resize(info.data.size());
	VkDeviceSize totalSize = info.data[0].size * info.data[0].count;
	result.dataOffsets[0] = std::make_pair(info.data[0].id, 0);
	for (int64_t i = 1; i < info.data.size(); ++i) {
		int padding = totalSize % info.data[i].size == 0 ? 0 : info.data[i].size * (totalSize / info.data[i].size + 1) - totalSize;
		totalSize += padding;
		result.dataOffsets[i] = std::make_pair(info.data[i].id, totalSize / info.data[i].size);
		totalSize += info.data[i].size * info.data[i].count;
	}

	auto padMod = totalSize % info.deviceOffsetAlignment;
	totalSize += padMod == 0 ? 0 : glm::abs(info.deviceOffsetAlignment - padMod);

	result.dynamicOffset = totalSize;
	result.buffer = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, info.binding, 0,
		totalSize * info.sizeMultiplier, info.stages | VK_SHADER_STAGE_FRAGMENT_BIT, info.name);

	return result;
}

Graphics::SceneGraphicsInfo* Scene::createDescriptors(size_t numFrameImages, const VkPhysicalDeviceLimits& limits)
{
	const LogicDevice* logicDevice = masters_->getLogicDevice();
	Descriptor* descriptor = masters_->getLogicDevice()->getPrimaryDescriptor();
	std::unordered_map<RendererTypes, uint32_t> instancesMap;
	findDescriptorRequirements(instancesMap);

	auto mvpResult = coolDescriptor({ numFrameImages, limits.minStorageBufferOffsetAlignment, 0, "MVPBuffer", VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		{ 
			{ (size_t)RendererTypes::kStatic, sizeof(glm::mat4), instancesMap[RendererTypes::kStatic] }, 
			{ (size_t)RendererTypes::kTerrain, sizeof(glm::mat4), instancesMap[RendererTypes::kTerrain] },
			{ (size_t)RendererTypes::kParticle, sizeof(glm::mat4), instancesMap[RendererTypes::kParticle] } 
		} }, logicDevice);
	graphicsInfo_.mvpBuffer = mvpResult.buffer;
	graphicsInfo_.mvpRange = mvpResult.dynamicOffset;

	auto paramsResult = coolDescriptor({ numFrameImages, limits.minStorageBufferOffsetAlignment, 1, "ParamsBuffer", 
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		{ 
			{ (size_t)RendererTypes::kStatic, sizeof(StaticShaderParams), instancesMap[RendererTypes::kStatic] }, 
			{ (size_t)RendererTypes::kTerrain, sizeof(StaticShaderParams), instancesMap[RendererTypes::kTerrain] },
			{ (size_t)RendererTypes::kParticle, sizeof(ParticleShaderParams), instancesMap[RendererTypes::kParticle] },
			{ (size_t)RendererTypes::kAtmosphere, sizeof(AtmosphereShaderParams), instancesMap[RendererTypes::kAtmosphere] }
		} }, logicDevice);
	graphicsInfo_.paramsBuffer = paramsResult.buffer;
	graphicsInfo_.paramsRange = paramsResult.dynamicOffset;
	
	auto materialResult = coolDescriptor({ numFrameImages, limits.minStorageBufferOffsetAlignment, 2, "MaterialsBuffer", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		{ 
			{ (size_t)RendererTypes::kStatic, sizeof(Materials::Static), instancesMap[RendererTypes::kStatic] }, 
			{ (size_t)RendererTypes::kTerrain, sizeof(Materials::Terrain), instancesMap[RendererTypes::kTerrain] },
			{ (size_t)RendererTypes::kParticle, sizeof(Materials::Particle), instancesMap[RendererTypes::kParticle] },
			{ (size_t)RendererTypes::kPostProcess, sizeof(Materials::PostProcess), 1 }
		} }, logicDevice);
	graphicsInfo_.materialBuffer = materialResult.buffer;
	graphicsInfo_.materialRange = materialResult.dynamicOffset;

	for (size_t i = 0; i < mvpResult.dataOffsets.size(); ++i) {
		graphicsInfo_.mvpOffsetSizes[mvpResult.dataOffsets[i].first] = mvpResult.dataOffsets[i].second;
	}
	for (size_t i = 0; i < paramsResult.dataOffsets.size(); ++i) {
		graphicsInfo_.paramsOffsetSizes[paramsResult.dataOffsets[i].first] = paramsResult.dataOffsets[i].second;
	}
	for (size_t i = 0; i < materialResult.dataOffsets.size(); ++i) {
		graphicsInfo_.materialOffsetSizes[materialResult.dataOffsets[i].first] = materialResult.dataOffsets[i].second;
	}
	
	graphicsInfo_.layout = descriptor->makeLayout({ graphicsInfo_.mvpBuffer->getBinding(), graphicsInfo_.paramsBuffer->getBinding(), graphicsInfo_.materialBuffer->getBinding() });
	graphicsInfo_.set = descriptor->getSet(descriptor->createSets({ graphicsInfo_.layout }));

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(graphicsInfo_.mvpBuffer->descriptorWrite(graphicsInfo_.set, 0, graphicsInfo_.mvpRange));
	descWrites.push_back(graphicsInfo_.paramsBuffer->descriptorWrite(graphicsInfo_.set, 0, graphicsInfo_.paramsRange));
	descWrites.push_back(graphicsInfo_.materialBuffer->descriptorWrite(graphicsInfo_.set, 0, graphicsInfo_.materialRange));
	descriptor->updateDescriptorSets(descWrites);

	graphicsWriteInfo_.graphicsMVPData.resize(graphicsInfo_.mvpRange);
	graphicsWriteInfo_.graphicsParamsData.resize(graphicsInfo_.paramsRange);
	graphicsWriteInfo_.graphicsMaterialData.resize(graphicsInfo_.materialRange);

	return &graphicsInfo_;
}

SceneHeirarchyNode* Scene::findEntityNodeRecursively(Entity* entity, SceneHeirarchyNode* node)
{
	if (node->entity == entity) {
		return node;
	}
	else {
		for (size_t i = 0; i < node->childNodes.size(); ++i) {
			auto childNode = findEntityNodeRecursively(entity, node->childNodes[i]);
			if (childNode != nullptr) {
				return childNode;
			}
		}
		return nullptr;
	}
}

void Scene::deleteHeirarchyRecursively(SceneHeirarchyNode* node)
{
	// Delete nodes from the bottom of the heirarchy up.
	for (size_t i = 0; i < node->childNodes.size(); ++i) {
		deleteHeirarchyRecursively(node->childNodes[i]);
	}
	if (node->parentNode != nullptr) {
		auto parentChildren = node->parentNode->childNodes;
		parentChildren.erase(std::find(parentChildren.begin(), parentChildren.end(), node));
		node->entity->setSceneNode(nullptr);
	}
	SAFE_DELETE(node->entity);
	SAFE_DELETE(node);
}

void Scene::updateRecursively(SceneHeirarchyNode* node, glm::mat4& viewProjection, glm::mat4 ctm, float dt, const uint32_t& frameIdx, LogicalCamera& mainCamera)
{
	// Update might cause the entity to move, therefore calculate the concatenated model matrix after updating
	node->entity->update(dt, viewProjection, ctm);
	// The final model matrix of the entity accounts for the transforms of itself and all parents
	glm::mat4 m = ctm * node->entity->getTransform()->toModelMatrix();
	node->entity->setModelMatrix(m);
	if (node->entity->getGraphicsComponent() != nullptr) {
		writeGraphicsData(node->entity->getGraphicsComponent(), viewProjection, m, frameIdx);
		addToCommandList(node->entity->getGraphicsComponent(), mainCamera); // TODO view frustum cull
	}
	for (size_t i = 0; i < node->childNodes.size(); ++i) {
		updateRecursively(node->childNodes[i], viewProjection, m, dt, frameIdx, mainCamera);
	}
}

void Scene::startRecursively(SceneHeirarchyNode* node)
{
	node->entity->start();
	auto graphicsComponent = node->entity->getGraphicsComponent();
	if (graphicsComponent != nullptr) {
		if (graphicsComponent->getRendererType() == Graphics::RendererTypes::kParticle) {
			graphicsComponent->setMesh(static_cast<Game::ParticleSystem*>(node->entity->getGameScript())->makeMesh());
		}
		else {
			if (!(kRendererTypeFlags[(size_t)graphicsComponent->getRendererType()] & RendererFlags::FULLSCREEN)) {
				auto buffer = masters_->graphicsMaster->getDynamicBuffer(graphicsComponent->getRendererType());
				graphicsComponent->setMesh(MeshLoader::loadMesh(graphicsComponent->getMeshName(), *buffer, graphicsComponent->getLoadInfo()));
			}
		}
	}
	for (size_t i = 0; i < node->childNodes.size(); ++i) {
		startRecursively(node->childNodes[i]);
	}
}

void Scene::findDescriptorRequirementsRecursively(std::unordered_map<Graphics::RendererTypes, uint32_t>& instancesCount, SceneHeirarchyNode* node)
{
	if (node->entity->getGraphicsComponent() != nullptr) {
		instancesCount[node->entity->getGraphicsComponent()->getRendererType()]++;
	}
	for (size_t i = 0; i < node->childNodes.size(); ++i) {
		findDescriptorRequirementsRecursively(instancesCount, node->childNodes[i]);
	}
}

void Scene::addToCommandList(Graphics::GraphicsComponent* component, LogicalCamera& mainCamera)
{
	auto rtype = component->getRendererType();
	if (!(kRendererTypeFlags[(size_t)rtype] & RendererFlags::FULLSCREEN)) {
		BasicMesh* mesh = component->getMesh();
		if (kRendererTypeFlags[(size_t)rtype] & RendererFlags::NON_INDEXED) {
			graphicsCommandLists_[(size_t)rtype].push_back({ mesh->count, 1, 0, mesh->vertexOffset, (uint32_t)graphicsCommandLists_[(size_t)rtype].size() });
		}
		else {
			graphicsCommandLists_[(size_t)rtype].push_back({ mesh->count, 1, mesh->indexOffset, mesh->vertexOffset, (uint32_t)graphicsCommandLists_[(size_t)rtype].size() });
		}

		graphicsWriteInfo_.distances[(size_t)rtype].push_back(glm::distance(component->getEntity()->getTransform()->position, mainCamera.position));
	}
}

void Scene::writeGraphicsData(Graphics::GraphicsComponent* component, glm::mat4& viewProjection, glm::mat4& ctm, const uint32_t& frameIdx)
{
	auto rtype = component->getRendererType();
	if (kRendererTypeFlags[(size_t)rtype] & RendererFlags::DESCRIPTOR_MVP) {
		auto offset = (graphicsInfo_.mvpOffsetSizes[(size_t)rtype] + graphicsWriteInfo_.offsets[(size_t)rtype]) * sizeof(glm::mat4);
		auto mvp = viewProjection * ctm;
		std::memcpy(&graphicsWriteInfo_.graphicsMVPData.data()[offset], (char*)&mvp, sizeof(glm::mat4));
	}
	if (kRendererTypeFlags[(size_t)rtype] & RendererFlags::DESCRIPTOR_PARAMS) {
		ShaderParams* tmpParams = component->getShaderParams();
		size_t paramsSize = ShaderParams::shaderParamsLUT[(size_t)rtype];
		if (kRendererTypeFlags[(size_t)rtype] & RendererFlags::INCLUDE_MODEL) {
			std::memcpy((char*)tmpParams, (char*)&ctm, sizeof(glm::mat4));
		}
		auto offset = (graphicsWriteInfo_.offsets[(size_t)rtype] + graphicsInfo_.paramsOffsetSizes[(size_t)rtype]) * paramsSize;
		std::memcpy(&graphicsWriteInfo_.graphicsParamsData.data()[offset], (char*)tmpParams, paramsSize);
	}
	if (kRendererTypeFlags[(size_t)rtype] & RendererFlags::DESCRIPTOR_MATERIAL) {
		Material* tmpMaterial = component->getMaterial();
		size_t materialSize = Materials::materialSizeLUT[(size_t)rtype];
		auto offset = (graphicsInfo_.materialOffsetSizes[(size_t)rtype] + graphicsWriteInfo_.offsets[(size_t)rtype]) * materialSize;
		std::memcpy(&graphicsWriteInfo_.graphicsMaterialData.data()[offset], (char*)tmpMaterial->data, tmpMaterial->size);
	}
	++graphicsWriteInfo_.offsets[(size_t)rtype];
}

std::ostream& outputSceneRecursively(std::ostream& os, SceneHeirarchyNode* node, std::string depthIndicator) {
	os << depthIndicator << node->entity->name() << std::endl;
	for (auto child : node->childNodes) {
		outputSceneRecursively(os, child, depthIndicator + "-");
	}
	return os;
}

// Display the entire scene graph
std::ostream& QZL::operator<<(std::ostream& os, Scene* scene)
{
	os << "Root node" << std::endl;
	for (auto child : scene->rootNode_->childNodes) {
		outputSceneRecursively(os, child, "-");
	}
	return os;
}
