// Author: Ralph Ridley
// Date: 11/11/19
#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"
#include "Material.h"

namespace QZL {
	namespace Graphics {
		class DescriptorBuffer;
		class ElementBufferObject;
		struct SceneGraphicsInfo {
			size_t numFrameIndices;
			VkDescriptorSet set;
			VkDescriptorSetLayout layout;

			VkDeviceSize mvpOffsetSizes[(size_t)RendererTypes::kNone];
			DescriptorBuffer* mvpBuffer;
			VkDeviceSize mvpRange;

			DescriptorBuffer* paramsBuffer;
			VkDeviceSize paramsRange;
			VkDeviceSize paramsOffsetSizes[(size_t)RendererTypes::kNone];

			DescriptorBuffer* materialBuffer;
			VkDeviceSize materialRange;
			VkDeviceSize materialOffsetSizes[(size_t)RendererTypes::kNone];

			ElementBufferObject* shadowCastingEBOs[(size_t)RendererTypes::kNone];
		};
	}
}
