#pragma once
#include "../Graphics/VkUtil.h"
#include "../Graphics/GraphicsTypes.h"

namespace QZL {
	class Entity;
	struct SceneHeirarchyNode {
		SceneHeirarchyNode* parentNode;
		Entity* entity;
		std::vector<SceneHeirarchyNode*> childNodes;
	};
	// Encompasses a game scene, defining entities in a tree heirarchy with pointers to both parent and children.
	class Scene {
		friend std::ostream& operator<<(std::ostream& os, Scene* scene);
	public:
		Scene();
		~Scene();
		// Calls update on every entity in the scene hierarchy, giving a combined model matrix such that
		// parents are the spatial root of their children.
		void update(float dt);

		void start();
		/*  
			Add an entity to the scene heirarchy with a given parent entity. Parent must exist in the scene heirarchy
			or be nullptr for the root node. The entity to add must not be nullptr and must not already exist in the heirarchy.
			If hintNode is not nullptr then it must be the node holding the parent entity, and allows skipping
			heirarchy traversal.
			Returns the node that is created.
		*/
		SceneHeirarchyNode* addEntity(Entity* entity, Entity* parent = nullptr, SceneHeirarchyNode* hintNode = nullptr);

		/*
			Removes the given entity from the scene heirarchy. If reparent is false then the entity's node and all child nodes are removed.
			If true then the heirarchy reparents all children to the entity's parent. Entity must not be nullptr, but may not exist in the heirarchy.
			A hintNode can be provided and is assumed to be the entity's node, this allows skipping heirarchy traversal.
		*/
		void removeEntity(Entity* entity, bool reparent = false, SceneHeirarchyNode* hintNode = nullptr);

		// Searches for the given entity in the scene heirarchy and returns its node.
		SceneHeirarchyNode* findEntityNode(Entity* entity);

		void findDescriptorRequirements(std::unordered_map<Graphics::RendererTypes, uint32_t>& instancesCount);

	private:
		// Auxilliary recursive lookup
		SceneHeirarchyNode* findEntityNodeRecursively(Entity* entity, SceneHeirarchyNode* node);
		// Deletes the given node and all of its children
		void deleteHeirarchyRecursively(SceneHeirarchyNode* node);

		void updateRecursively(SceneHeirarchyNode* node, glm::mat4 modelMatrix, float dt);
		void startRecursively(SceneHeirarchyNode* node);

		void findDescriptorRequirementsRecursively(std::unordered_map<Graphics::RendererTypes, uint32_t>& instancesCount, SceneHeirarchyNode* node);

		SceneHeirarchyNode* rootNode_;
	};
}
