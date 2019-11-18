// Author: Ralph Ridley
// Date: 01/11/19

#include "ParticleRenderer.h"
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

ParticleRenderer::ParticleRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new DynamicElementBuffer(createInfo.logicDevice->getDeviceMemory(), createInfo.swapChainImageCount, sizeof(ParticleVertex)))
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	VkPushConstantRange pushConstants[2] = {
		setupPushConstantRange<CameraPushConstants>(VK_SHADER_STAGE_VERTEX_BIT),
		setupPushConstantRange<TessellationPushConstants>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
	};

	uint32_t offsets[3] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kParticle], graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kParticle], graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kParticle] };
	std::vector<VkSpecializationMapEntry> mapEntry = {
		makeSpecConstantEntry(0, 0,	sizeof(uint32_t)),
		makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t))
	};
	auto geomSpecConstant = setupSpecConstants(2, mapEntry.data(), sizeof(uint32_t) * 2, &offsets[0]);
	auto fragSpecConstant = setupSpecConstants(2, mapEntry.data(), sizeof(uint32_t) * 2, &offsets[1]);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);
	stageInfos.emplace_back(createInfo.geometryShader, VK_SHADER_STAGE_GEOMETRY_BIT, &geomSpecConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Particle";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<ParticleVertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), 2, pushConstants), stageInfos, pci);
}

void ParticleRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	if (commandList->size() == 0)
		return;
	beginFrame(cmdBuffer);
	ebo_->updateBuffer(cmdBuffer, idx);
	bindEBO(cmdBuffer, idx);
	for (auto& cmd : *commandList) {
		vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance);
	}
}
