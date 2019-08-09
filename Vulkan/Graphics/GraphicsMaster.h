#pragma once
#include "VkUtil.h"
#include "../InputManager.h"
#include "OptionalExtentions.h"

namespace QZL
{
	struct SystemMasters;
	namespace Game {
		class GameMaster;
	}
	namespace Graphics {
		enum class ElementBufferTypes {
			STATIC
		};

		enum class RendererTypes {
			STATIC, TERRAIN, ATMOSPHERE
		};

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

		class GraphicsMaster;
		class Validation;

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
			void registerComponent(GraphicsComponent* component);
			void setRenderer(RendererTypes type, RendererBase* renderer);
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
			void initDevices(uint32_t& enabledLayerCount, const char* const*& enabledLayerNames);

			void preframeSetup();

			void loop();

			GraphicsSystemDetails details_;
			Validation* validation_;

			std::unordered_map<RendererTypes, RendererBase*> renderers_;

			glm::mat4* viewMatrix_;
			glm::vec3* camPosition_;

			InputProfile inputProfile_;

			const SystemMasters& masters_;

			static const int kDefaultWidth = 800;
			static const int kDefaultHeight = 600;
		};
	}
}
