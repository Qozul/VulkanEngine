// Author: Ralph Ridley
// Date: 23/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		class IndexedRenderer : public RendererBase {
		public:
			IndexedRenderer(RendererCreateInfo2& createInfo2, LogicDevice* logicDevice, VkRenderPass renderPass, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo);
			~IndexedRenderer() = default;
			void recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList, bool ignoreEboBind = false) override;
		};
	}
}
