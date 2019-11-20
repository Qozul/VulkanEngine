// Author: Ralph Ridley
// Date: 20/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class WaterRenderer : public RendererBase {
		public:
			WaterRenderer(RendererCreateInfo& createInfo);
			~WaterRenderer() = default;
			void recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList) override;
		};
	}
}
