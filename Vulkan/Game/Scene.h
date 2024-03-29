#pragma once
#include "../Graphics/VkUtil.h"
#include "../Graphics/GraphicsTypes.h"
#include "../Graphics/SceneDescriptorInfo.h"
#include "../Graphics/LogicalCamera.h"
#include "../Graphics/Light.h"

namespace QZL {
	class Entity;
	namespace Graphics {
		class GraphicsComponent;
		class GlobalRenderData;
		class LogicDevice;
		class DescriptorBuffer;
		struct LogicalCamera;
	}

	struct SceneHeirarchyNode {
		SceneHeirarchyNode* parentNode = nullptr;
		Entity* entity = nullptr;
		std::vector<SceneHeirarchyNode*> childNodes;
	};

	struct GraphicsWriteInfo {
		char* mvpPtr = nullptr;
		char* paramsPtr = nullptr;
		char* materialPtr = nullptr;
		VkDeviceSize offsets[(size_t)Graphics::RendererTypes::kNone] = {};
		std::vector<char> graphicsMVPData[NUM_CAMERAS];
		std::vector<char> graphicsParamsData;
		std::vector<char> graphicsMaterialData;
		std::vector<float> distances[(size_t)Graphics::RendererTypes::kNone];
		std::vector<Graphics::Light> lightData;
	};

	struct DescriptorData {
		int id = 0;
		VkDeviceSize size = 0;
		size_t count = 0;
	};

	struct DynamicDescriptorInput {
		VkDeviceSize sizeMultiplier = 0; // i.e. frameImageCount
		VkDeviceSize deviceOffsetAlignment = 0;
		uint32_t binding = 0;
		std::string name;
		VkShaderStageFlags stages = 0;
		std::vector<DescriptorData> data;
	};

	struct DynamicDescriptorInfo {
		Graphics::DescriptorBuffer* buffer = nullptr;
		VkDeviceSize dynamicOffset = 0;
		std::vector<std::pair<size_t, uint32_t>> dataOffsets;
	};

	// Encompasses a game scene, defining entities in a tree heirarchy with pointers to both parent and children.
	class Scene {
		friend std::ostream& operator<<(std::ostream& os, Scene* scene);
	public:
		Scene(const SystemMasters* masters);
		~Scene();
		// Calls update on every entity in the scene hierarchy, giving a combined model matrix such that
		// parents are the spatial root of their children.
		std::vector<VkDrawIndexedIndirectCommand>* update(Graphics::LogicalCamera* cameras, const size_t cameraCount, float dt, const uint32_t& frameIdx, Graphics::GlobalRenderData* grd);

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
		Graphics::SceneGraphicsInfo* createDescriptors(uint32_t numFrameImages, const VkPhysicalDeviceLimits& limits);

	private:
		// Auxilliary recursive lookup
		SceneHeirarchyNode* findEntityNodeRecursively(Entity* entity, SceneHeirarchyNode* node);
		// Deletes the given node and all of its children
		void deleteHeirarchyRecursively(SceneHeirarchyNode* node);

		void updateRecursively(SceneHeirarchyNode* node, Graphics::LogicalCamera* cameras, const size_t cameraCount, glm::mat4 ctm, float dt, const uint32_t& frameIdx);
		void startRecursively(SceneHeirarchyNode* node);

		void findDescriptorRequirementsRecursively(std::unordered_map<Graphics::RendererTypes, uint32_t>& instancesCount, SceneHeirarchyNode* node);
		DynamicDescriptorInfo makeDynamicDescriptor(DynamicDescriptorInput info, const Graphics::LogicDevice* logicDevice);
		void addDynamicDescriptor(Graphics::DescriptorBuffer*& buffer, size_t& range, uint32_t offsets[(size_t)Graphics::RendererTypes::kNone], std::vector<DescriptorData> data,
			uint32_t numFrameImages, VkDeviceSize alignment, uint32_t bindingIdx, std::string name, VkShaderStageFlags flags, const Graphics::LogicDevice* logicDevice);
		void addToCommandList(Graphics::GraphicsComponent* component, Graphics::LogicalCamera& mainCamera);
		void writeGraphicsData(Graphics::GraphicsComponent* component, Graphics::LogicalCamera* cameras, size_t cameraCount, glm::mat4& ctm, const uint32_t& frameIdx);
		void sort(Graphics::RendererTypes rtype);

		SceneHeirarchyNode* rootNode_;
		Graphics::SceneGraphicsInfo graphicsInfo_;
		GraphicsWriteInfo graphicsWriteInfo_;
		std::vector<VkDrawIndexedIndirectCommand> graphicsCommandLists_[(size_t)Graphics::RendererTypes::kNone];
		
		const SystemMasters* masters_;
	};
}
