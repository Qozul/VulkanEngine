// Author: Ralph Ridley
// Date: 01/11/19
#include "WaterRenderer.h"
#include "ElementBufferObject.h"
#include "GlobalRenderData.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "SceneDescriptorInfo.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

WaterRenderer::WaterRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex),
		sizeof(uint16_t)))
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	VkPushConstantRange pushConstants[1] = {
		setupPushConstantRange<VertexPushConstants>(VK_SHADER_STAGE_VERTEX_BIT)
	};

	uint32_t offsets[3] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kWater], graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kWater], graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kWater] };
	std::vector<VkSpecializationMapEntry> mapEntry = {
		makeSpecConstantEntry(0, 0,	sizeof(uint32_t)),
		makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t))
	};
	auto vertSpecConstant = setupSpecConstants(1, mapEntry.data(), sizeof(uint32_t), &offsets[1]);
	auto tescSpecConstant = setupSpecConstants(1, mapEntry.data(), sizeof(uint32_t), &offsets[1]);
	auto teseSpecConstant = setupSpecConstants(3, mapEntry.data(), sizeof(uint32_t) * 3, offsets);
	auto fragSpecConstant = setupSpecConstants(1, mapEntry.data(), sizeof(uint32_t), &offsets[1]);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, &vertSpecConstant);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);
	stageInfos.emplace_back(createInfo.tessControlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, &tescSpecConstant);
	stageInfos.emplace_back(createInfo.tessEvalShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, &teseSpecConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Water";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	pci.subpassIndex = createInfo.subpassIndex;
	pci.dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	pci.sampleCount = VK_SAMPLE_COUNT_8_BIT;

	createPipeline<Vertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), 1, pushConstants), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
}

void WaterRenderer::recordFrame(const uint32_t frameIdx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	if (commandList->size() == 0)
		return;
	beginFrame(cmdBuffer);
	ebo_->bind(cmdBuffer, frameIdx);
	for (auto& cmd : *commandList) {
		vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance);
	}
}