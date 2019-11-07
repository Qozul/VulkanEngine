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
	glm::mat4 inverseViewProj;
	glm::vec3 camPos;
	float Hatm;
	glm::vec3 sunDir;
	float planetRadius;
	glm::vec3 betaRay;
	float betaMie;
};

PostProcessRenderer::PostProcessRenderer(RendererCreateInfo& createInfo, TextureSampler* geometryColourBuffer, TextureSampler* geometryDepthBuffer)
	: RendererBase(createInfo, nullptr), geometryColourBuffer_(geometryColourBuffer), geometryDepthBuffer_(geometryDepthBuffer), component_(nullptr)
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
	VkDescriptorSetLayoutBinding gcbBinding = TextureSampler::makeBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding gdbBinding = TextureSampler::makeBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layout = descriptor_->makeLayout({ gcbBinding, gdbBinding });

	pipelineLayouts_.push_back(layout);
	pipelineLayouts_.push_back(AtmosphereMaterial::getLayout(descriptor_));

	descriptorSets_.push_back(descriptor_->getSet(descriptor_->createSets({ layout })));

	std::vector<VkWriteDescriptorSet> descWrites;
	descWrites.push_back(geometryColourBuffer_->descriptorWrite(descriptorSets_[0], 0));
	descWrites.push_back(geometryDepthBuffer_->descriptorWrite(descriptorSets_[0], 1));
	descriptor_->updateDescriptorSets(descWrites);
}

void PostProcessRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	beginFrame(cmdBuffer);

	VkDescriptorSet sets[2] = { descriptorSets_[0], material_->getTextureSet() };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, sets, 0, nullptr);

	PushConstants pc;
	pc.inverseViewProj = glm::inverse(GraphicsMaster::kProjectionMatrix * camera.viewMatrix);
	pc.camPos = camera.position;
	pc.Hatm = params_->params.Hatm;
	pc.sunDir = *params_->params.sunDirection;
	pc.planetRadius = params_->params.planetRadius;
	pc.betaRay = params_->params.betaRay;
	pc.betaMie = params_->params.betaMie;

	vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pc);

	vkCmdDrawIndexed(cmdBuffer, 3, 1, 0, 0, 0);
}

void PostProcessRenderer::registerComponent(GraphicsComponent* component, RenderObject* robject)
{
	params_ = static_cast<AtmosphereShaderParams*>(component->getPerMeshShaderParams());
	material_ = component->getMaterial();
	component_ = component;
}
