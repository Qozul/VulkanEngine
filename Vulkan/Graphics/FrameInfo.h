#pragma once
#include "LogicalCamera.h"

namespace QZL {
	namespace Graphics {
		struct FrameInfo {
			LogicalCamera cameras[NUM_CAMERAS];
			size_t mainCameraIdx;
			uint32_t frameIdx;
			int32_t viewportX;
			uint32_t viewportWidth;
			VkCommandBuffer cmdBuffer;
			std::vector<VkDrawIndexedIndirectCommand>* commandLists;
		};
	}
}
