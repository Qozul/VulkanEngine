// Author: Ralph Ridley
// Date: 21/11/19
#include "FullscreenRenderer.h"
#include "GlobalRenderData.h"
#include "SceneDescriptorInfo.h"

using namespace QZL;
using namespace QZL::Graphics;

FullscreenRenderer::FullscreenRenderer(RendererCreateInfo& createInfo, RendererCreateInfo2& createInfo2)
	: RendererBase(createInfo, nullptr)
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	createPipeline(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), createInfo2.pcRangesCount, createInfo2.pcRanges), createInfo2.shaderStages, createInfo2.pipelineCreateInfo);
}

void FullscreenRenderer::recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	beginFrame(cmdBuffer);
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
}
