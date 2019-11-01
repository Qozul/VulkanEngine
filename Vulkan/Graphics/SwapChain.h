#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

#define MAX_FRAMES_IN_FLIGHT 2

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class RenderPass;
		struct DeviceSurfaceCapabilities;
		class GlobalRenderData;
		class GraphicsMaster;
		struct LogicalCamera;

		struct SwapChainDetails {
			VkSwapchainKHR swapChain;
			VkSurfaceFormatKHR surfaceFormat;
			VkPresentModeKHR presentMode;
			VkExtent2D extent;
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;
		};

		class SwapChain {
			friend class GraphicsMaster;
		public:
			void loop(LogicalCamera& camera);

			static size_t numSwapChainImages;
		private:
			SwapChain(GraphicsMaster* master, GLFWwindow* window, VkSurfaceKHR surface, LogicDevice* logicDevice, DeviceSurfaceCapabilities& surfaceCapabilities);
			~SwapChain();

			RenderPass* getRenderPass(RenderPassTypes type) {
				return renderPasses_[(size_t)type];
			}

			// Heavily similar to tutorial code
			void initSwapChain(GLFWwindow* window, DeviceSurfaceCapabilities& surfaceCapabilities);
			void initSwapChainImages(GLFWwindow* window, VkSurfaceKHR surface, DeviceSurfaceCapabilities& surfaceCapabilities);
			void initImageViews();

			// These three functions are straight from the tutorial 
			VkSurfaceFormatKHR chooseFormat(std::vector<VkSurfaceFormatKHR>& formats);
			VkPresentModeKHR choosePresentMode(std::vector<VkPresentModeKHR>& presentModes);
			VkExtent2D chooseExtent(GLFWwindow* window, VkSurfaceCapabilitiesKHR& capabilities);

			void setCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers);

			// Also very much similar to tutorial code
			uint32_t aquireImage();
			void submitQueue(const uint32_t imgIdx, VkSemaphore signalSemaphores[]);
			void present(const uint32_t imgIdx, VkSemaphore signalSemaphores[]);

			GlobalRenderData* globalRenderData_;

			std::vector<VkCommandBuffer> commandBuffers_;
			std::vector<RenderPass*> renderPasses_;

			SwapChainDetails details_;
			LogicDevice* logicDevice_;
			GraphicsMaster* master_;

			void createSyncObjects();
			std::vector<VkSemaphore> imageAvailableSemaphores_;
			std::vector<VkSemaphore> renderFinishedSemaphores_;
			std::vector<VkFence> inFlightFences_;
			size_t currentFrame_ = 0;
		};
	}
}
