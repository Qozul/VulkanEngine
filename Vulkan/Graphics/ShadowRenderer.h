// Author: Ralph Ridley
// Date: 18/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		// Draws atmospheres from within the atmosphere
		class ShadowRenderer : public RendererBase {
		public:
			ShadowRenderer(RendererCreateInfo& createInfo);
			~ShadowRenderer() = default;
			void recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList) override;
		};
	}
}
