// Author: Ralph Ridley
// Date: 01/11/19

#include "AtmosphereRenderer.h"
#include "ElementBufferObject.h"
#include "GlobalRenderData.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "RendererPipeline.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "SceneDescriptorInfo.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

AtmosphereRenderer::AtmosphereRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, nullptr)
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	VkPushConstantRange pushConstants[1] = {
		setupPushConstantRange<VertexPushConstants>(VK_SHADER_STAGE_VERTEX_BIT)
	};

	uint32_t offsets[1] = { graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kAtmosphere] };
	std::vector<VkSpecializationMapEntry> mapEntry = {
		makeSpecConstantEntry(0, 0,	sizeof(uint32_t))
	};
	auto fragSpecConstant = setupSpecConstants(1, mapEntry.data(), sizeof(uint32_t), offsets);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Atmosphere";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pci.sampleCount = VK_SAMPLE_COUNT_1_BIT;

	createPipeline(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data(), 1, pushConstants), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
}

void AtmosphereRenderer::recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	beginFrame(cmdBuffer);
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
}
