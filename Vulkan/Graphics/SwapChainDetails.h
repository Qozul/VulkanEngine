// Author: Ralph Ridley
// Date: 04/11/19
#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Graphics {
		struct SwapChainDetails {
			VkSwapchainKHR swapChain;
			VkFormat depthFormat;
			VkSurfaceFormatKHR surfaceFormat;
			VkPresentModeKHR presentMode;
			VkExtent2D extent;
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;
		};
	}
}
