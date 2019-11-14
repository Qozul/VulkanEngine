// Author: Ralph Ridley
// Date: 01/11/19

#include "AtmosphereRenderer.h"
#include "ElementBufferObject.h"
#include "RenderStorage.h"
#include "GlobalRenderData.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "RendererPipeline.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

struct PushConstantExtent {
	glm::mat4 inverseViewProj;
	glm::vec3 betaRay;
	float betaMie = 0.0f;
	glm::vec3 cameraPosition;
	float planetRadius = 0.0f;
	glm::vec3 sunDirection;
	float Hatm = 0.0f;
	glm::vec3 sunIntensity;
	uint32_t samplerIdx = 0;
};

AtmosphereRenderer::AtmosphereRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), 
		sizeof(VertexOnlyPosition), sizeof(uint16_t)), RenderStorage::InstanceUsage::kOne))
{
	createDescriptors(createInfo.maxDrawnEntities);
	descriptorSets_.push_back(createInfo.globalRenderData->getSet());
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	auto pushConstRange = setupPushConstantRange<PushConstantExtent>(VK_SHADER_STAGE_FRAGMENT_BIT);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.debugName = "Atmosphere";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<VertexOnlyPosition>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data(), 1, &pushConstRange), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
}

AtmosphereRenderer::~AtmosphereRenderer()
{
}

void AtmosphereRenderer::createDescriptors(const uint32_t entityCount)
{
}

void AtmosphereRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;

	beginFrame(cmdBuffer);
	bindEBO(cmdBuffer, idx);

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		auto perMeshParams = static_cast<AtmosphereShaderParams*>(robject->getParams());
		AtmosphereShaderParams::Params params = perMeshParams->params;

		auto vm = glm::lookAt({ 0.0f, camera.position.y, 0.0f }, camera.lookPoint + glm::vec3(0.0f, camera.position.y, 0.0f), { 0.0f, 1.0f, 0.0f });
		PushConstantExtent pce;
		pce.inverseViewProj = glm::inverse(GraphicsMaster::kProjectionMatrix * vm);
		pce.cameraPosition = camera.position;
		pce.sunDirection = *params.sunDirection;
		pce.sunIntensity = *params.sunIntensity;
		pce.betaMie = params.betaMie;
		pce.betaRay = params.betaRay;
		pce.samplerIdx = *static_cast<uint32_t*>(robject->getMaterial()->data);
		pce.Hatm = params.Hatm;
		pce.planetRadius = params.planetRadius;

		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, &descriptorSets_[0], 0, nullptr);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pce);
		
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
