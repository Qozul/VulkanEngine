// Author: Ralph Ridley
// Date: 01/11/19

#include "AtmosphereRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "DeviceMemory.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "SwapChain.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "../Assets/Entity.h"
#include "../Game/SunScript.h"
#include "../Game/AtmosphereScript.h"

using namespace QZL;
using namespace Graphics;

struct PushConstantExtent {
	glm::mat4 inverseViewProj;
	glm::vec3 betaRay;
	float betaMie;
	glm::vec3 cameraPosition;
	float planetRadius;
	glm::vec3 sunDirection;
	float Hatm;
	glm::vec3 sunIntensity;
	float g;
};

AtmosphereRenderer::AtmosphereRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBuffer<VertexOnlyPosition>(createInfo.logicDevice->getDeviceMemory()), RenderStorage::InstanceUsage::ONE))
{
	createDescriptors(createInfo.maxDrawnEntities);

	auto pushConstRange = setupPushConstantRange<PushConstantExtent>(VK_SHADER_STAGE_FRAGMENT_BIT);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<VertexOnlyPosition>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data(), 1, &pushConstRange), 
		stageInfos, pci, RendererPipeline::PrimitiveType::QUADS);
}

AtmosphereRenderer::~AtmosphereRenderer()
{
}

void AtmosphereRenderer::createDescriptors(const uint32_t entityCount)
{
	pipelineLayouts_.push_back(AtmosphereMaterial::getLayout(descriptor_));
}

void AtmosphereRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;

	beginFrame(cmdBuffer);
	renderStorage_->buffer()->bind(cmdBuffer, idx);

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		auto perMeshParams = static_cast<AtmosphereShaderParams*>(robject->getParams());
		AtmosphereShaderParams::Params params = perMeshParams->params;

		PushConstantExtent pce;
		pce.inverseViewProj = glm::inverse(GraphicsMaster::kProjectionMatrix * camera.viewMatrix);
		pce.cameraPosition = camera.position;
		pce.sunDirection = *params.sunDirection;
		pce.sunIntensity = *params.sunIntensity;
		pce.betaMie = params.betaMie;
		pce.betaRay = params.betaRay;
		pce.g = params.g;
		pce.Hatm = params.Hatm;
		pce.planetRadius = params.planetRadius;
		VkDescriptorSet sets[1] = { robject->getMaterial()->getTextureSet() };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, sets, 0, nullptr);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pce);
		
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
