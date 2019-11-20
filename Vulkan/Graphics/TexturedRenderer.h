// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class TexturedRenderer : public RendererBase {
		public:
			TexturedRenderer(RendererCreateInfo& createInfo);
			~TexturedRenderer() = default;
			void recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList) override;
		};
	}
}
