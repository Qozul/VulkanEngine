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
#include "SceneDescriptorInfo.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

AtmosphereRenderer::AtmosphereRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), 
		sizeof(VertexOnlyPosition), sizeof(uint16_t)), RenderStorage::InstanceUsage::kOne))
{
	descriptorSets_.push_back(createInfo.graphicsInfo->set);
	descriptorSets_.push_back(createInfo.globalRenderData->getSet());
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());
	storageBuffers_.push_back(createInfo.graphicsInfo->paramsBuffer);

	VkPushConstantRange pushConstants[2] = {
		setupPushConstantRange<CameraPushConstants>(VK_SHADER_STAGE_VERTEX_BIT),
		setupPushConstantRange<TessellationPushConstants>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
	};

	uint32_t offsets[1] = { graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kAtmosphere] };
	std::vector<VkSpecializationMapEntry> mapEntry = {
		makeSpecConstantEntry(0, 0,	sizeof(uint32_t))
	};
	auto fragSpecConstant = setupSpecConstants(1, mapEntry.data(), sizeof(uint32_t), offsets);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Atmosphere";
	pci.enableDepthTest = VK_FALSE;
	pci.enableDepthWrite = VK_FALSE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<VertexOnlyPosition>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data(), 2, pushConstants), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
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

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpRange * idx,
		graphicsInfo_->paramsRange * idx,
		graphicsInfo_->materialRange * idx
	};

	auto vm = glm::lookAt({ 0.0f, camera.position.y, 0.0f }, camera.lookPoint + glm::vec3(0.0f, camera.position.y, 0.0f), { 0.0f, 1.0f, 0.0f });
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		AtmosphereShaderParams* params = static_cast<AtmosphereShaderParams*>(robject->getParams());
		AtmosphereShaderParams* paramsPtr = (AtmosphereShaderParams*)(static_cast<char*>(storageBuffers_[0]->bindRange()) +
			sizeof(AtmosphereShaderParams) * graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kAtmosphere] + dynamicOffsets[1]);
		paramsPtr[i].betaMie = params->betaMie;
		paramsPtr[i].betaRay = params->betaRay;
		paramsPtr[i].g = params->g;
		paramsPtr[i].Hatm = params->Hatm;
		paramsPtr[i].planetRadius = params->planetRadius;
		paramsPtr[i].sunDirection = glm::vec3(0.0f, 1.0f, 0.0f);
		paramsPtr[i].sunIntensity = glm::vec3(6.5e-7, 5.1e-7, 4.75e-7) * glm::vec3(1e7);
		paramsPtr[i].inverseViewProj = glm::inverse(GraphicsMaster::kProjectionMatrix * vm);
		paramsPtr[i].scatteringIdx = 8;
		storageBuffers_[0]->unbindRange();
		
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
