// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class TerrainRenderer : public RendererBase {
		public:
			TerrainRenderer(RendererCreateInfo& createInfo);
			~TerrainRenderer() = default;
			void recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList) override;
		};
	}
}
