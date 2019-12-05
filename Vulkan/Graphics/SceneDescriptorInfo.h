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
			uint32_t numFrameIndices = 0;
			VkDescriptorSet set = VK_NULL_HANDLE;
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;

			size_t mvpRange = 0;
			uint32_t mvpOffsetSizes[(size_t)RendererTypes::kNone];
			DescriptorBuffer* mvpBuffer = nullptr;

			size_t paramsRange = 0;
			uint32_t paramsOffsetSizes[(size_t)RendererTypes::kNone];
			DescriptorBuffer* paramsBuffer = nullptr;

			size_t materialRange = 0;
			uint32_t materialOffsetSizes[(size_t)RendererTypes::kNone];
			DescriptorBuffer* materialBuffer = nullptr;

			ElementBufferObject* shadowCastingEBOs[(size_t)RendererTypes::kNone];
			DescriptorBuffer* lightsBuffer = nullptr;
		};
	}
}
