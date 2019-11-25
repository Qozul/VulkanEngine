#pragma once
#include "LogicalCamera.h"

namespace QZL {
	namespace Graphics {
		struct FrameInfo {
			LogicalCamera cameras[NUM_CAMERAS];
			size_t mainCameraIdx;
			float sunHeight;
			uint32_t frameIdx;
			int32_t viewportX;
			uint32_t viewportWidth;
			bool splitscreenEnabled;
			VkCommandBuffer cmdBuffer;
			std::vector<VkDrawIndexedIndirectCommand>* commandLists;
		};
	}
}
