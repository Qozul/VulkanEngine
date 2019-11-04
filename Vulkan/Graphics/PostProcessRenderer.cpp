// Author: Ralph Ridley
// Date: 01/11/19

#include "PostProcessRenderer.h"
#include "GlobalRenderData.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "RendererPipeline.h"
#include "GraphicsMaster.h"

using namespace QZL;
using namespace Graphics;

PostProcessRenderer::PostProcessRenderer(RendererCreateInfo& createInfo, TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer)
	: RendererBase(createInfo, nullptr), geometryColourBuffer_(geometryColourBuffer), geometryDepthBuffer_(geometryDepthBuffer)
{
	createDescriptors(createInfo.maxDrawnEntities);

	std::array<VkSpecializationMapEntry, 2> specConstantEntries;
	specConstantEntries[0].constantID = 0;
	specConstantEntries[0].size = sizeof(float);
	specConstantEntries[0].offset = 0;
	specConstantEntries[1].constantID = 1;
	specConstantEntries[1].size = sizeof(float);
	specConstantEntries[1].offset = sizeof(float);
	std::array<float, 2> specConstantValues;
	specConstantValues[0] = GraphicsMaster::NEAR_PLANE_Z;
	specConstantValues[1] = GraphicsMaster::FAR_PLANE_Z;

	std::array<VkSpecializationInfo, 2> specConstants;
	specConstants[0].mapEntryCount = 0;
	specConstants[0].dataSize = 0;
	specConstants[1].mapEntryCount = static_cast<uint32_t>(specConstantEntries.size());
	specConstants[1].pMapEntries = specConstantEntries.data();
	specConstants[1].dataSize = sizeof(specConstantValues);
	specConstants[1].pData = specConstantValues.data();

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, &specConstants[0]);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &specConstants[1]);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), pipelineLayouts_.data(), 0, nullptr), stageInfos, pci,
		RendererPipeline::PrimitiveType::kQuads);
}

PostProcessRenderer::~PostProcessRenderer()
{
}

void PostProcessRenderer::createDescriptors(const uint32_t entityCount)
{
	VkDescriptorSetLayoutBinding gcbBinding = TextureSampler::makeBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding gdbBinding = TextureSampler::makeBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layout = descriptor_->makeLayout({ gcbBinding, gdbBinding });

	pipelineLayouts_.push_back(layout);

	descriptorSets_.push_back(descriptor_->getSet(descriptor_->createSets({ layout })));

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(geometryColourBuffer_->descriptorWrite(descriptorSets_[0], 0));
	descWrites.push_back(geometryDepthBuffer_->descriptorWrite(descriptorSets_[0], 1));
	descriptor_->updateDescriptorSets(descWrites);
}

void PostProcessRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	beginFrame(cmdBuffer);

	VkDescriptorSet sets[1] = { descriptorSets_[0] };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, sets, 0, nullptr);

	vkCmdDrawIndexed(cmdBuffer, 3, 1, 0, 0, 0);
}
