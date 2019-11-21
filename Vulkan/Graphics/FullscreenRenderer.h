// Author: Ralph Ridley
// Date: 21/11/19
#pragma once
#include "RendererBase.h"

namespace QZL
{
	namespace Graphics {
		struct RendererCreateInfo2 {
			PipelineCreateInfo pipelineCreateInfo;
			std::vector<ShaderStageInfo> shaderStages;
			VkPushConstantRange* pcRanges;
			uint32_t pcRangesCount;
		};

		class FullscreenRenderer : public RendererBase {
		public:
			FullscreenRenderer(RendererCreateInfo& createInfo, RendererCreateInfo2& createInfo2);
			~FullscreenRenderer() = default;
			void recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList) override;
		};
	}
}
