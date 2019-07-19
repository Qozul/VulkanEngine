#pragma once
#include "VkUtil.h"

#define MAX_FRAMES_IN_FLIGHT 2

namespace QZL
{
	namespace Graphics {
		class LogicDevice;
		class RenderPass;
		struct DeviceSurfaceCapabilities;
		class GraphicsMaster;

		struct SwapChainDetails {
			VkSwapchainKHR swapChain;
			VkSurfaceFormatKHR surfaceFormat;
			VkPresentModeKHR presentMode;
			VkExtent2D extent;
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;
		};

		class SwapChain {
			friend class LogicDevice;
		public:
			void loop();
		private:
			SwapChain(GraphicsMaster* master, GLFWwindow* window, VkSurfaceKHR surface, LogicDevice* logicDevice, DeviceSurfaceCapabilities& surfaceCapabilities);
			~SwapChain();

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

			std::vector<VkCommandBuffer> commandBuffers_;
			RenderPass* renderPass_;

			SwapChainDetails details_;
			LogicDevice* logicDevice_;

			void createSyncObjects();
			std::vector<VkSemaphore> imageAvailableSemaphores_;
			std::vector<VkSemaphore> renderFinishedSemaphores_;
			std::vector<VkFence> inFlightFences_;
			size_t currentFrame_ = 0;
		};
	}
}
