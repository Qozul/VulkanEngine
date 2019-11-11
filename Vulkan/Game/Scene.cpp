#include "Scene.h"
#include "../Assets/Entity.h"
#include "../Assets/Transform.h"

using namespace QZL;

Scene::Scene()
{
	rootNode_ = new SceneHeirarchyNode();
	rootNode_->parentNode = nullptr;
	rootNode_->entity = nullptr;
}

Scene::~Scene()
{
	deleteHeirarchyRecursively(rootNode_);
}

void Scene::update(float dt)
{
	for (size_t i = 0; i < rootNode_->childNodes.size(); ++i) {
		updateRecursively(rootNode_->childNodes[i], glm::mat4(), dt);
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

void Scene::updateRecursively(SceneHeirarchyNode* node, glm::mat4 ctm, float dt)
{
	// Update might cause the entity to move, therefore calculate the concatenated model matrix after updating
	node->entity->update(dt, ctm);
	// The final model matrix of the entity accounts for the transforms of itself and all parents
	glm::mat4 m = ctm * node->entity->getTransform()->toModelMatrix();
	node->entity->setModelMatrix(m);
	for (size_t i = 0; i < node->childNodes.size(); ++i) {
		updateRecursively(node->childNodes[i], m, dt);
	}
}

void Scene::startRecursively(SceneHeirarchyNode* node)
{
	node->entity->start();
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
