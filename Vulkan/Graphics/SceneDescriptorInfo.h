// Author: Ralph Ridley
// Date: 11/11/19
#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"
#include "Material.h"

namespace QZL {
	namespace Graphics {
		class DescriptorBuffer;
		struct SceneGraphicsInfo {
			VkDescriptorSet set;
			VkDescriptorSetLayout layout;
			DescriptorBuffer* paramsBuffer;
			DescriptorBuffer* mvpBuffer;
			DescriptorBuffer* materialBuffer;
			VkDeviceSize paramsRange;
			VkDeviceSize mvpRange;
			VkDeviceSize materialRange;
			VkDeviceSize paramsOffsetSizes[(size_t)RendererTypes::kNone];
			VkDeviceSize mvpOffsetSizes[(size_t)RendererTypes::kNone];
			VkDeviceSize materialOffsetSizes[(size_t)RendererTypes::kNone];
		};
	}
}
