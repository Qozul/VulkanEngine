#include "Scene.h"
#include "../Assets/Entity.h"
#include "../Assets/Transform.h"
#include "../SystemMasters.h"
#include "../Graphics/GraphicsMaster.h"
#include "ParticleSystem.h"
#include "../Graphics/StorageBuffer.h"
#include "../Graphics/Descriptor.h"

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
}

void Scene::update(glm::mat4& viewProjection, float dt, const uint32_t& frameIdx)
{
	for (size_t i = 0; i < rootNode_->childNodes.size(); ++i) {
		updateRecursively(rootNode_->childNodes[i], viewProjection, glm::mat4(), dt, frameIdx);
	}
}

void Scene::start()
{
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

struct IN {
	VkDeviceSize sizeMultiplier; // i.e. frameImageCount
	VkDeviceSize deviceOffsetAlignment;
	uint32_t binding;
	std::string name;
	VkShaderStageFlags stages;
	std::vector<std::pair<VkDeviceSize, VkDeviceSize>> data;
};

struct OUT_YEAH {
	DescriptorBuffer* buffer;
	VkDeviceSize dynamicOffset;
	std::vector<VkDeviceSize> dataOffsets; // In same order as passed in
};

bool compareFunc(std::pair<VkDeviceSize, VkDeviceSize>& a, std::pair<VkDeviceSize, VkDeviceSize>& b)
{
	return a.first > b.first;
}

OUT_YEAH coolDescriptor(IN info, const LogicDevice* logicDevice) {
	ASSERT_DEBUG(info.data.size() > 0);
	OUT_YEAH result;
	result.dataOffsets.resize(info.data.size());
	result.dataOffsets[0] = 0;
	VkDeviceSize totalSize = 0;
	if (info.data.size() == 1) {
		totalSize = info.data[0].first * info.data[0].second;
	}
	else {
		std::sort(info.data.begin(), info.data.end(), compareFunc);
		for (int64_t i = 0; i < info.data.size() - 1; ++i) {
			VkDeviceSize segmentSize = info.data[i].first * info.data[i].second;
			int padding = info.data[i + 1].first * (segmentSize / info.data[i + 1].first + 1) - segmentSize;
			segmentSize += padding;
			result.dataOffsets[i + 1] = segmentSize / info.data[i + 1].first;
		}
	}
	totalSize += glm::abs(info.deviceOffsetAlignment - totalSize % info.deviceOffsetAlignment);
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

	VkDeviceSize Rs0, Rs1;
	VkDeviceSize Rt0, Rt1;
	VkDeviceSize Ot0, Ot1;

	// ------------------------------- COMBINED MVP --------
	{
		Rs0 = sizeof(glm::mat4) * instancesMap[RendererTypes::kStatic];
		Rt0 = sizeof(glm::mat4) * instancesMap[RendererTypes::kTerrain];
		VkDeviceSize Os = 0; // Store offsets in graphics info for correct update ranges
		Ot0 = Rs0;
		VkDeviceSize Rtotal = Rs0 + Rt0;
		VkDeviceSize padding = glm::abs(limits.minStorageBufferOffsetAlignment - Rtotal % limits.minStorageBufferOffsetAlignment);
		Rtotal += padding; // Rtotal is the dynamic offset

		graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
			Rtotal * numFrameImages, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "MVPBuffer");

		graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kStatic] = Rtotal;
		graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kTerrain] = Rtotal;
	}
	// ------------------------------- COMBINED Params --------
	{
		Rs1 = sizeof(StaticShaderParams) * instancesMap[RendererTypes::kStatic];
		Rt1 = sizeof(StaticShaderParams) * instancesMap[RendererTypes::kTerrain];
		VkDeviceSize Os = 0;
		Ot1 = Rs1;
		VkDeviceSize Rtotal = Rs1 + Rt1;
		VkDeviceSize padding = glm::abs(limits.minStorageBufferOffsetAlignment - Rtotal % limits.minStorageBufferOffsetAlignment);
		Rtotal += padding;

		graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 1, 0,
			Rtotal * numFrameImages, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "ParamsBuffer");

		graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kStatic] = Rtotal;
		graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kTerrain] = Rtotal;
	}
	// ------------------------------- COMBINED Materials --------
	
	auto result = coolDescriptor({ numFrameImages, limits.minStorageBufferOffsetAlignment, 2, "MaterialsBuffer", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		{ { sizeof(Materials::Static), instancesMap[RendererTypes::kStatic] }, { sizeof(Materials::Terrain), instancesMap[RendererTypes::kTerrain] } } }, logicDevice);
	graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic] = result.buffer;
	graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kTerrain] = result.dataOffsets[1];

	// ------------------------------------------------------------
	
	auto layout = descriptor->makeLayout({ graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->getBinding(),
			graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic]->getBinding(), graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic]->getBinding() });
	graphicsInfo_.layouts[(size_t)RendererTypes::kStatic] = layout;
	graphicsInfo_.sets[(size_t)RendererTypes::kStatic] = descriptor->createSets({ layout });

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), 0, Rs0 + Rt0));
	descWrites.push_back(graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), 0, Rs1 + Rt1));
	descWrites.push_back(graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), 0, result.dynamicOffset));
	descriptor->updateDescriptorSets(descWrites);

	// -------------------------------- STATIC ---------
	/*{
		auto layout = descriptor->makeLayout({ graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->getBinding(),
			graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic]->getBinding(), graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic]->getBinding() });
		graphicsInfo_.layouts[(size_t)RendererTypes::kStatic] = layout;
		graphicsInfo_.sets[(size_t)RendererTypes::kStatic] = descriptor->createSets({ layout });

		std::vector<VkWriteDescriptorSet> descWrites;
		descWrites.push_back(graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), 0, Rs0));
		descWrites.push_back(graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), 0, Rs1));
		descWrites.push_back(graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), 0, Rs2));
		descriptor->updateDescriptorSets(descWrites);
	}

	// ----------------------------------------------------

	// -------------------------------- TERRAIN ---------
	{
		auto layout = descriptor->makeLayout({ graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->getBinding(),
			graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic]->getBinding(), graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic]->getBinding() });
		graphicsInfo_.layouts[(size_t)RendererTypes::kTerrain] = layout;
		graphicsInfo_.sets[(size_t)RendererTypes::kTerrain] = descriptor->createSets({ layout });

		std::vector<VkWriteDescriptorSet> descWrites;
		descWrites.push_back(graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kTerrain]), Ot0, Rt0));
		descWrites.push_back(graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kTerrain]), Ot1, Rt1));
		descWrites.push_back(graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kTerrain]), Ot2, Rt2));
		descriptor->updateDescriptorSets(descWrites);
	}*/
	// -----------------------------------------------------

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

void Scene::updateRecursively(SceneHeirarchyNode* node, glm::mat4& viewProjection, glm::mat4 ctm, float dt, const uint32_t& frameIdx)
{
	// Update might cause the entity to move, therefore calculate the concatenated model matrix after updating
	node->entity->update(dt, ctm);
	// The final model matrix of the entity accounts for the transforms of itself and all parents
	glm::mat4 m = ctm * node->entity->getTransform()->toModelMatrix();
	node->entity->setModelMatrix(m);
	for (size_t i = 0; i < node->childNodes.size(); ++i) {
		updateRecursively(node->childNodes[i], viewProjection, m, dt, frameIdx);
	}
}

void Scene::startRecursively(SceneHeirarchyNode* node)
{
	node->entity->start();
	auto graphicsComponent = node->entity->getGraphicsComponent();
	if (graphicsComponent != nullptr) {
		if (graphicsComponent->getRendererType() == Graphics::RendererTypes::kParticle) {
			masters_->graphicsMaster->registerComponent(graphicsComponent, static_cast<Game::ParticleSystem*>(node->entity->getGameScript())->makeRenderObject(node->entity->name()));
		}
		else {
			masters_->graphicsMaster->registerComponent(graphicsComponent);
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
