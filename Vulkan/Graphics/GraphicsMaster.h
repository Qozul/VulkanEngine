#pragma once
#include "VkUtil.h"
#include "../InputManager.h"
#include "OptionalExtentions.h"
#include "GraphicsTypes.h"

namespace QZL
{
	struct SystemMasters;
	namespace Game {
		class GameMaster;
		class AtmosphereScript;
	}
	namespace Graphics {

		struct EnvironmentArgs {
			int numObjectsX;
			int numObjectsY;
			int numObjectsZ;
		};

		struct DeviceSwapChainDetails;
		class PhysicalDevice;
		class LogicDevice;
		class CommandPool;
		class GraphicsComponent;
		class RendererBase;
		struct BasicMesh;
		struct DeviceSurfaceCapabilities;
		class GraphicsMaster;
		class Validation;
		class SwapChain;

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

			void registerComponent(GraphicsComponent* component);
			void setRenderer(RendererTypes type, RendererBase* renderer);
			void attachPostProcessScript(Game::AtmosphereScript* script);
			glm::mat4* getViewMatrixPtr() {
				return viewMatrix_;
			}
			glm::vec3* getCamPosPtr() {
				return camPosition_;
			}
			const glm::mat4& getViewMatrix() {
				return *viewMatrix_;
			}
			const glm::vec3& getCamPos() {
				return *camPosition_;
			}
			const LogicDevice* getLogicDevice() {
				return details_.logicDevice;
			}
			const SystemMasters& getMasters() {
				return masters_;
			}
			const bool supportsOptionalExtension(OptionalExtensions ext);
			static glm::mat4 kProjectionMatrix;
		private:
			GraphicsMaster(const SystemMasters& masters);
			~GraphicsMaster();

			void initGlfw(std::vector<const char*>& extensions);
			void initInstance(const std::vector<const char*>& extensions, uint32_t& enabledLayerCount,
				const char* const*& enabledLayerNames);
			void initDevices(DeviceSurfaceCapabilities& surfaceCapabilitie, uint32_t& enabledLayerCount, const char* const*& enabledLayerNames);

			void preframeSetup();

			void loop();

			GraphicsSystemDetails details_;
			Validation* validation_;

			SwapChain* swapChain_;

			std::unordered_map<RendererTypes, RendererBase*> renderers_;

			glm::mat4* viewMatrix_;
			glm::vec3* camPosition_;

			InputProfile inputProfile_;

			const SystemMasters& masters_;

			static const int kDefaultWidth = 800;
			static const int kDefaultHeight = 600;
			static const int MAX_DESCRIPTOR_INDEXED_TEXTURES = 2;
		};
	}
}
