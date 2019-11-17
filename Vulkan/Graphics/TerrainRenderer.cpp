// Author: Ralph Ridley
// Date: 01/11/19
#include "TerrainRenderer.h"
#include "ElementBufferObject.h"
#include "RenderStorage.h"
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

TerrainRenderer::TerrainRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), 
		sizeof(uint16_t))))
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	VkPushConstantRange pushConstants[2] = {
		setupPushConstantRange<CameraPushConstants>(VK_SHADER_STAGE_VERTEX_BIT),
		setupPushConstantRange<TessellationPushConstants>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
	};

	uint32_t offsets[3] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kTerrain], graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kTerrain], graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kTerrain] };
	std::vector<VkSpecializationMapEntry> mapEntry = {
		makeSpecConstantEntry(0, 0,	sizeof(uint32_t)),
		makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t))
	};
	auto tescSpecConstant = setupSpecConstants(1, mapEntry.data(), sizeof(uint32_t), &offsets[2]);
	auto teseSpecConstant = setupSpecConstants(3, mapEntry.data(), sizeof(uint32_t) * 3, offsets);
	auto fragSpecConstant = setupSpecConstants(2, mapEntry.data(), sizeof(uint32_t) * 2, &offsets[1]);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);
	stageInfos.emplace_back(createInfo.tessControlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, &tescSpecConstant);
	stageInfos.emplace_back(createInfo.tessEvalShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, &teseSpecConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Terrain";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<Vertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data(), 2, pushConstants), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
}

void TerrainRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	if (renderStorage_->meshCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buffer()->bind(cmdBuffer, idx); 
	for (auto& cmd : *commandList) {
		vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance);
	}
}
