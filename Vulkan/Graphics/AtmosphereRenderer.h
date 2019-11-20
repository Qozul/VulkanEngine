// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		// Draws atmospheres from within the atmosphere
		class AtmosphereRenderer : public RendererBase {
		public:
			AtmosphereRenderer(RendererCreateInfo& createInfo);
			~AtmosphereRenderer() = default;
			void recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList) override;
		};
	}
}
