// Author: Ralph Ridley
// Date: 21/11/19
#include "FullscreenRenderer.h"
#include "GlobalRenderData.h"
#include "SceneDescriptorInfo.h"

using namespace QZL;
using namespace QZL::Graphics;

FullscreenRenderer::FullscreenRenderer(RendererCreateInfo2& createInfo2, LogicDevice* logicDevice, VkRenderPass renderPass, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RendererBase(logicDevice, nullptr, graphicsInfo)
{
	pipelineLayouts_.push_back(graphicsInfo->layout);
	pipelineLayouts_.push_back(grd->getLayout());

	createPipeline(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), createInfo2.pcRangesCount, createInfo2.pcRanges), createInfo2.shaderStages, createInfo2.pipelineCreateInfo, RendererPipeline::PrimitiveType::kNone);
}

void FullscreenRenderer::recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList, bool ignoreEboBind)
{
	beginFrame(cmdBuffer);
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
}
