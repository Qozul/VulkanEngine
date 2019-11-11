// Author: Ralph Ridley
// Date: 11/11/19
#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

namespace QZL {
	namespace Graphics {
		class DescriptorBuffer;
		struct SceneDescriptorInfo {
			size_t mvpOffsets[(size_t)RendererTypes::kNone];
			size_t paramsOffsets[(size_t)RendererTypes::kNone];
			VkDescriptorSetLayout materialsLayout;
			VkDescriptorSetLayout instanceDataLayout;
			DescriptorBuffer* materialBuffer;
			DescriptorBuffer* paramsBuffer;
			DescriptorBuffer* mvpBuffer;
		};
	}
}
