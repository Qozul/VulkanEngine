#include "IndexedRenderer.h"
#include "ElementBufferObject.h"
#include "GlobalRenderData.h"
#include "SceneDescriptorInfo.h"

using namespace QZL;
using namespace QZL::Graphics;

IndexedRenderer::IndexedRenderer(RendererCreateInfo2& createInfo2, LogicDevice* logicDevice, VkRenderPass renderPass, GlobalRenderData* grd, SceneGraphicsInfo* graphicsInfo)
	: RendererBase(logicDevice, createInfo2.ebo, graphicsInfo)
{
	pipelineLayouts_.push_back(graphicsInfo->layout);
	pipelineLayouts_.push_back(grd->getLayout());

	createPipeline(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), createInfo2.pcRangesCount, createInfo2.pcRanges), createInfo2.shaderStages, createInfo2.pipelineCreateInfo, createInfo2.tessellationPrims, createInfo2.vertexTypes);
}

void IndexedRenderer::recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList, bool ignoreEboBind)
{
	if (commandList->size() == 0)
		return;
	beginFrame(cmdBuffer);
	if (!ignoreEboBind) ebo_->bind(cmdBuffer, frameIdx);

	for (auto& cmd : *commandList) {
		vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance);
	}
}
