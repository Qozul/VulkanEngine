// Author: Ralph Ridley
// Date: 01/11/19
#include "TexturedRenderer.h"
#include "ElementBufferObject.h"
#include "RenderStorage.h"
#include "GlobalRenderData.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "ShaderParams.h"
#include "Material.h"
#include "RenderObject.h"
#include "SceneDescriptorInfo.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

TexturedRenderer::TexturedRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), sizeof(uint16_t)), 
		RenderStorage::InstanceUsage::kUnlimited))
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());
	
	VkPushConstantRange pushConstants[2] = { 
		setupPushConstantRange<CameraPushConstants>(VK_SHADER_STAGE_VERTEX_BIT), 
		setupPushConstantRange<TessellationPushConstants>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) 
	};

	uint32_t offsets[3] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kStatic], graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kStatic], graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kStatic] };
	std::vector<VkSpecializationMapEntry> mapEntry = {
		makeSpecConstantEntry(0, 0,	sizeof(uint32_t)),
		makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t))
	};
	auto vertSpecConstant = setupSpecConstants(2, mapEntry.data(), sizeof(uint32_t) * 2, &offsets[0]);
	auto fragSpecConstant = setupSpecConstants(2, mapEntry.data(), sizeof(uint32_t) * 2, &offsets[1]);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, &vertSpecConstant);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Static";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<Vertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data(), 2, pushConstants), stageInfos, pci);
}

void TexturedRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buffer()->bind(cmdBuffer, idx);

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
