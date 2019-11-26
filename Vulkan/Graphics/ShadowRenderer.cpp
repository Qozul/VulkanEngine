// Author: Ralph Ridley
// Date: 01/11/19

#include "ShadowRenderer.h"
#include "DynamicElementBuffer.h"
#include "StorageBuffer.h"
#include "GlobalRenderData.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "SceneDescriptorInfo.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

struct PushConstants {
	uint32_t mvpOffset;
	uint32_t heightmapIdx;
};

ShadowRenderer::ShadowRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, nullptr)
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	VkPushConstantRange pushConstants[1] = {
		setupPushConstantRange<PushConstants>(VK_SHADER_STAGE_VERTEX_BIT)
	};

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	if (createInfo.tessControlShader != "") {
		stageInfos.emplace_back(createInfo.tessControlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, nullptr);
		stageInfos.emplace_back(createInfo.tessEvalShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, nullptr);
	}

	PipelineCreateInfo pci = {};
	pci.debugName = "Shadow";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.cullFace = VK_CULL_MODE_NONE;
	pci.primitiveTopology = createInfo.prims;
	pci.subpassIndex = createInfo.subpassIndex;
	pci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pci.depthBiasEnable = VK_TRUE;
	pci.colourAttachmentCount = 0;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	createPipeline<Vertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), 1, pushConstants), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
}

void ShadowRenderer::recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	if (commandList->size() == 0)
		return;
	beginFrame(cmdBuffer);
	for (auto& cmd : *commandList) {
		vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance);
	}
}
