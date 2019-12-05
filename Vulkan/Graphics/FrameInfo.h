#pragma once
#include "LogicalCamera.h"

namespace QZL {
	namespace Graphics {
		struct FrameInfo {
			LogicalCamera cameras[NUM_CAMERAS];
			uint32_t mainCameraIdx = 0;
			float sunHeight = 0.0f;
			uint32_t frameIdx = 0;
			int32_t viewportX = 0;
			uint32_t viewportWidth = 0;
			bool splitscreenEnabled = false;
			VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
			std::vector<VkDrawIndexedIndirectCommand>* commandLists;
		};
	}
}
