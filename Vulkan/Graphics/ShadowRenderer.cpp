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
};

ShadowRenderer::ShadowRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, nullptr)
{
	VkPushConstantRange pushConstants[1] = {
		setupPushConstantRange<CameraPushConstants>(VK_SHADER_STAGE_VERTEX_BIT)
	};

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.debugName = "Particle";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), 1, pushConstants), stageInfos, pci);
}

void ShadowRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	if (commandList->size() == 0)
		return;
	beginFrame(cmdBuffer);
	for (auto& cmd : *commandList) {
		vkCmdDrawIndexed(cmdBuffer, cmd.indexCount, cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset, cmd.firstInstance);
	}
}
