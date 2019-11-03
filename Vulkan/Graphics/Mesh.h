// Author: Ralph Ridley
// Date: 03/11/19
#pragma once
#include "VkUtil.h"

namespace QZL
{
	namespace Graphics {
		// BasicMesh needs to provide a transform and pointers to it's data
		struct BasicMesh {
			uint32_t count; // This will be index count for indexed data, vertex count otherwise
			uint32_t indexOffset; // Index offset is only used for indexed data
			uint32_t vertexOffset;
		};
	}
}