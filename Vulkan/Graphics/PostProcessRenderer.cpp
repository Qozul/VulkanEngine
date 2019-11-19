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

PostProcessRenderer::PostProcessRenderer(RendererCreateInfo& createInfo, uint32_t geometryColourTexture)
	: RendererBase(createInfo, nullptr), geometryColourTexture_(geometryColourTexture)
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());
	storageBuffers_.push_back(createInfo.graphicsInfo->materialBuffer);

	VkPushConstantRange pushConstants[1] = {
		setupPushConstantRange<VertexPushConstants>(VK_SHADER_STAGE_VERTEX_BIT)
	};

	struct Vals {
		float nearPlane;
		float farPlane;
		uint32_t offset;
	} specConstantValues;
	specConstantValues.nearPlane = GraphicsMaster::NEAR_PLANE_Z;
	specConstantValues.farPlane = GraphicsMaster::FAR_PLANE_Z;
	specConstantValues.offset = graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kPostProcess];
	std::vector<VkSpecializationMapEntry> specEntries = { 
		makeSpecConstantEntry(0, 0, sizeof(float)),
		makeSpecConstantEntry(1, sizeof(float), sizeof(float)), 
		makeSpecConstantEntry(2, sizeof(uint32_t) + sizeof(float), sizeof(uint32_t)) 
	};
	VkSpecializationInfo specializationInfo = setupSpecConstants(3, specEntries.data(), sizeof(Vals), &specConstantValues);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);

	PipelineCreateInfo pci = {};
	pci.debugName = "PostProcess";
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

void PostProcessRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer, std::vector<VkDrawIndexedIndirectCommand>* commandList)
{
	beginFrame(cmdBuffer);

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpRange * idx,
		graphicsInfo_->paramsRange * idx,
		graphicsInfo_->materialRange * idx
	};
	uint32_t* dataPtr = (uint32_t*)((char*)storageBuffers_[0]->bindRange() + sizeof(Materials::PostProcess) * graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kPostProcess] + dynamicOffsets[2]);
	dataPtr[0] = geometryColourTexture_;
	storageBuffers_[0]->unbindRange();

	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
}
