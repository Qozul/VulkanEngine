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
			/*size_t mvpOffsets[(size_t)RendererTypes::kNone];
			size_t paramsOffsets[(size_t)RendererTypes::kNone];
			VkDescriptorSetLayout materialsLayout;
			VkDescriptorSetLayout instanceDataLayout;
			DescriptorBuffer* materialBuffer;
			DescriptorBuffer* paramsBuffer;
			DescriptorBuffer* mvpBuffer;*/

			size_t sets[(size_t)RendererTypes::kNone];
			VkDescriptorSetLayout layouts[(size_t)RendererTypes::kNone];
			DescriptorBuffer* paramsBuffers[(size_t)RendererTypes::kNone];
			DescriptorBuffer* mvpBuffer[(size_t)RendererTypes::kNone];
			DescriptorBuffer* materialBuffer[(size_t)MaterialType::kSize];
		};
	}
}
