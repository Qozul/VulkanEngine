// Author: Ralph Ridley
// Date: 01/11/19

#include "PostProcessRenderer.h"
#include "GlobalRenderData.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "RendererPipeline.h"
#include "GraphicsMaster.h"
#include "RenderObject.h"
#include "ShaderParams.h"
#include "GraphicsComponent.h"
#include "SceneDescriptorInfo.h"

using namespace QZL;
using namespace Graphics;

PostProcessRenderer::PostProcessRenderer(RendererCreateInfo& createInfo, uint32_t texture)
	: RendererBase(createInfo, nullptr)
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	VkPushConstantRange pushConstants[1] = {
		setupPushConstantRange<VertexPushConstants>(VK_SHADER_STAGE_VERTEX_BIT)
	};

	uint32_t colourIdx = texture;
	std::vector<VkSpecializationMapEntry> specEntries = { 
		makeSpecConstantEntry(0, 0, sizeof(uint32_t)),
	};
	VkSpecializationInfo specializationInfo = setupSpecConstants(1, specEntries.data(), sizeof(uint32_t), &colourIdx);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);

	PipelineCreateInfo pci = {};
	pci.debugName = "PassThrough";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	createPipeline(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data(), 1, pushConstants), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
}

void PostProcessRenderer::recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	beginFrame(cmdBuffer);
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
}
