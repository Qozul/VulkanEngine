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

Graphics::SceneGraphicsInfo* Scene::createDescriptors(size_t numFrameImages, const VkPhysicalDeviceLimits& limits)
{
	const LogicDevice* logicDevice = masters_->getLogicDevice();
	Descriptor* descriptor = masters_->getLogicDevice()->getPrimaryDescriptor();
	std::unordered_map<RendererTypes, uint32_t> instancesMap;
	findDescriptorRequirements(instancesMap);


	// ------------------------------- COMBINED --------
	VkDeviceSize Rs = sizeof(glm::mat4) * instancesMap[RendererTypes::kStatic];
	VkDeviceSize Rt = sizeof(glm::mat4) * instancesMap[RendererTypes::kTerrain];
	VkDeviceSize Os = 0; // Store offstes in graphics info for correct update ranges
	VkDeviceSize Ot = Rs;
	VkDeviceSize Rtotal = Rs + Rt;
	VkDeviceSize padding = glm::abs(limits.minStorageBufferOffsetAlignment - Rtotal % limits.minStorageBufferOffsetAlignment);
	Rtotal += padding; // Rtotal is the dynamic offset

	graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
		Rtotal * numFrameImages, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "MVPBuffer");
	
	graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kStatic] = Rtotal;
	graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kTerrain] = Rtotal;
	
	// ------------------------------------------------

	// -------------------------------- STATIC ---------
	{
		auto staticCount = instancesMap[RendererTypes::kStatic];
		graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kStatic] = sizeof(glm::mat4) * staticCount;
		graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kStatic] = sizeof(StaticShaderParams) * staticCount;
		graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kStatic] = sizeof(uint32_t) * 2 * staticCount;
		VkDeviceSize mvpPaddingPerOffset = glm::abs(limits.minStorageBufferOffsetAlignment -
			graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kStatic] % limits.minStorageBufferOffsetAlignment);
		VkDeviceSize paramsPaddingPerOffset = glm::abs(limits.minStorageBufferOffsetAlignment -
			graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kStatic] % limits.minStorageBufferOffsetAlignment);
		VkDeviceSize materialPaddingPerOffset = glm::abs(limits.minStorageBufferOffsetAlignment -
			graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kStatic] % limits.minStorageBufferOffsetAlignment);
		graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kStatic] += mvpPaddingPerOffset;
		graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kStatic] += paramsPaddingPerOffset;
		graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kStatic] += materialPaddingPerOffset;

		//graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
		//	graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kStatic] * numFrameImages + (mvpPaddingPerOffset * (numFrameImages - 1)), VK_SHADER_STAGE_VERTEX_BIT, "StaticMVPBuffer");
		graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 1, 0,
			graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kStatic] * numFrameImages + (paramsPaddingPerOffset * (numFrameImages - 1)), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "StaticParamsBuffer");
		graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 2, 0,
			graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kStatic] * numFrameImages, VK_SHADER_STAGE_FRAGMENT_BIT, "StaticDIBuffer");
		auto layout = descriptor->makeLayout({ graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->getBinding(),
			graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic]->getBinding(), graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic]->getBinding() });
		graphicsInfo_.layouts[(size_t)RendererTypes::kStatic] = layout;
		graphicsInfo_.sets[(size_t)RendererTypes::kStatic] = descriptor->createSets({ layout });

		std::vector<VkWriteDescriptorSet> descWrites;
			descWrites.push_back(graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), Os, Rs));
			descWrites.push_back(graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), 0, graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kStatic]));
			descWrites.push_back(graphicsInfo_.materialBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kStatic]), 0, graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kStatic]));
		descriptor->updateDescriptorSets(descWrites);
	}

	// ----------------------------------------------------

	// -------------------------------- TERRAIN ---------
	{
		auto staticCount = instancesMap[RendererTypes::kTerrain];
		graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kTerrain] = sizeof(glm::mat4) * staticCount;
		graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kTerrain] = sizeof(StaticShaderParams) * staticCount;
		graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kTerrain] = sizeof(uint32_t) * 3 * staticCount;
		VkDeviceSize mvpPaddingPerOffset = glm::abs(limits.minStorageBufferOffsetAlignment - 
			graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kTerrain] % limits.minStorageBufferOffsetAlignment);
		VkDeviceSize paramsPaddingPerOffset = glm::abs(limits.minStorageBufferOffsetAlignment - 
			graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kTerrain] % limits.minStorageBufferOffsetAlignment);
		VkDeviceSize materialPaddingPerOffset = glm::abs(limits.minStorageBufferOffsetAlignment - 
			graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kTerrain] % limits.minStorageBufferOffsetAlignment);
		graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kTerrain] += mvpPaddingPerOffset;
		graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kTerrain] += paramsPaddingPerOffset;
		graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kTerrain] += materialPaddingPerOffset;

		//graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kTerrain] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
		//	graphicsInfo_.mvpOffsetSizes[(size_t)RendererTypes::kTerrain] * numFrameImages + (mvpPaddingPerOffset * (numFrameImages - 1)), VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "StaticMVPBuffer");
		graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kTerrain] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 1, 0,
			graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kTerrain] * numFrameImages + (paramsPaddingPerOffset * (numFrameImages - 1)), VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "StaticParamsBuffer");
		graphicsInfo_.materialBuffer[(size_t)RendererTypes::kTerrain] = DescriptorBuffer::makeBuffer<DynamicStorageBuffer>(logicDevice, MemoryAllocationPattern::kDynamicResource, 2, 0,
			graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kTerrain] * numFrameImages, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, "StaticDIBuffer");
		auto layout = descriptor->makeLayout({ graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->getBinding(),
			graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kTerrain]->getBinding(), graphicsInfo_.materialBuffer[(size_t)RendererTypes::kTerrain]->getBinding() });
		graphicsInfo_.layouts[(size_t)RendererTypes::kTerrain] = layout;
		graphicsInfo_.sets[(size_t)RendererTypes::kTerrain] = descriptor->createSets({ layout });

		std::vector<VkWriteDescriptorSet> descWrites;
		descWrites.push_back(graphicsInfo_.mvpBuffer[(size_t)RendererTypes::kStatic]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kTerrain]), Ot, Rt));
		descWrites.push_back(graphicsInfo_.paramsBuffers[(size_t)RendererTypes::kTerrain]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kTerrain]), 0, graphicsInfo_.paramsOffsetSizes[(size_t)RendererTypes::kTerrain]));
		descWrites.push_back(graphicsInfo_.materialBuffer[(size_t)RendererTypes::kTerrain]->descriptorWrite(descriptor->getSet(graphicsInfo_.sets[(size_t)RendererTypes::kTerrain]), 0, graphicsInfo_.materialOffsetSizes[(size_t)RendererTypes::kTerrain]));
		descriptor->updateDescriptorSets(descWrites);
	}
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
