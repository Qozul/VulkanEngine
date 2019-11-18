#pragma once
#include "VkUtil.h"
#include "../InputManager.h"
#include "GraphicsTypes.h"
#include "LogicalCamera.h"

namespace QZL
{
	class Scene;
	struct SystemMasters;
	namespace Game {
		class GameMaster;
		class AtmosphereScript;
	}
	namespace Graphics {
		class PhysicalDevice;
		class LogicDevice;
		class CommandPool;
		class GraphicsComponent;
		class RendererBase;
		class GraphicsMaster;
		class Validation;
		class SwapChain;
		class RenderObject;
		class ElementBufferObject;
		struct BasicMesh;
		struct DeviceSurfaceCapabilities;
		struct DeviceSwapChainDetails;
		struct SceneGraphicsInfo;

		struct GraphicsSystemDetails {
			GLFWwindow* window = nullptr;
			VkSurfaceKHR surface = VK_NULL_HANDLE;
			VkInstance instance = VK_NULL_HANDLE;

			PhysicalDevice* physicalDevice = nullptr;
			LogicDevice* logicDevice = nullptr;
			GraphicsMaster* master = nullptr;
		};

		class GraphicsMaster final {
			friend class System;
			friend class Game::GameMaster;
		public:
			static constexpr float NEAR_PLANE_Z = 0.1f;
			static constexpr float FAR_PLANE_Z = 1000.0f;

			void setRenderer(RendererTypes type, RendererBase* renderer);
			ElementBufferObject* getDynamicBuffer(RendererTypes type);

			LogicalCamera* getCamera(size_t idx);
			const LogicDevice* getLogicDevice() {
				return details_.logicDevice;
			}
			const SystemMasters& getMasters() {
				return masters_;
			}
			const bool supportsOptionalExtension(OptionalExtensions ext);
		private:
			GraphicsMaster(SystemMasters& masters);
			~GraphicsMaster();

			void initGlfw(std::vector<const char*>& extensions);
			void initInstance(const std::vector<const char*>& extensions, uint32_t& enabledLayerCount,
				const char* const*& enabledLayerNames);
			void initDevices(DeviceSurfaceCapabilities& surfaceCapabilitie, uint32_t& enabledLayerCount, const char* const*& enabledLayerNames);

			void initialiseRenderPath(Scene* scene, SceneGraphicsInfo* graphicsInfo);
			void preframeSetup();

			void loop();

			GraphicsSystemDetails details_;
			Validation* validation_;
			SwapChain* swapChain_;
			std::unordered_map<RendererTypes, RendererBase*> renderers_;
			InputProfile inputProfile_;
			const SystemMasters& masters_;

			static const int kDefaultWidth = 800;
			static const int kDefaultHeight = 600;
			static const int MAX_DESCRIPTOR_INDEXED_TEXTURES = 2;
		};
	}
}
