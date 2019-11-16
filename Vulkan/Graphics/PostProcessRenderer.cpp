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

using namespace QZL;
using namespace Graphics;

struct PushConstants {
	uint32_t colourIdx;
	uint32_t depthIdx;
};

PostProcessRenderer::PostProcessRenderer(RendererCreateInfo& createInfo, uint32_t geometryColourBuffer, uint32_t geometryDepthBuffer)
	: RendererBase(createInfo, nullptr), geometryColourBuffer_(geometryColourBuffer), geometryDepthBuffer_(geometryDepthBuffer)
{
	createDescriptors(createInfo.maxDrawnEntities);
	descriptorSets_.push_back(createInfo.globalRenderData->getSet());
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	std::array<float, 2> specConstantValues;
	specConstantValues[0] = GraphicsMaster::NEAR_PLANE_Z;
	specConstantValues[1] = GraphicsMaster::FAR_PLANE_Z;
	std::vector<VkSpecializationMapEntry> specEntries = { makeSpecConstantEntry(0, 0, sizeof(float)), makeSpecConstantEntry(0, sizeof(float), sizeof(float)) };
	VkSpecializationInfo specializationInfo = setupSpecConstants(2, specEntries.data(), sizeof(float) * 2, specConstantValues.data());

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &specializationInfo);

	auto pushConstRange = setupPushConstantRange<PushConstants>(VK_SHADER_STAGE_FRAGMENT_BIT);

	PipelineCreateInfo pci = {};
	pci.debugName = "PostProcess";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data(), 1, &pushConstRange), stageInfos, pci,
		RendererPipeline::PrimitiveType::kQuads);
}

PostProcessRenderer::~PostProcessRenderer()
{
}

void PostProcessRenderer::createDescriptors(const uint32_t entityCount)
{
}

void PostProcessRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	beginFrame(cmdBuffer);

	VkDescriptorSet sets[2] = { descriptorSets_[0] };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, sets, 0, nullptr);

	PushConstants pc;
	pc.colourIdx = geometryColourBuffer_;
	pc.depthIdx = geometryDepthBuffer_;

	vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pc);

	vkCmdDrawIndexed(cmdBuffer, 3, 1, 0, 0, 0);
}

void PostProcessRenderer::registerComponent(GraphicsComponent* component, RenderObject* robject)
{
}
