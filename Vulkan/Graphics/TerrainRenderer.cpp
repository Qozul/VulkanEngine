// Author: Ralph Ridley
// Date: 01/11/19
#include "TerrainRenderer.h"
#include "ElementBufferObject.h"
#include "RenderStorage.h"
#include "GlobalRenderData.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "ShaderParams.h"
#include "RenderObject.h"
#include "Material.h"
#include "SceneDescriptorInfo.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace Graphics;

struct TessCtrlPushConstants {
	float distanceFarMinusClose = 0.0f;
	float closeDistance = 0.0f;
	float patchRadius = 0.0f;
	float maxTessellationWeight = 0.0f;
	std::array<glm::vec4, 6> frustumPlanes;
};

TerrainRenderer::TerrainRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), 
		sizeof(uint16_t)), RenderStorage::InstanceUsage::kUnlimited))
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layouts[(size_t)RendererTypes::kStatic]);
	descriptorSets_.push_back(descriptor_->getSet(createInfo.graphicsInfo->sets[(size_t)RendererTypes::kStatic]));
	storageBuffers_.push_back(createInfo.graphicsInfo->mvpBuffer[(size_t)RendererTypes::kStatic]);
	storageBuffers_.push_back(createInfo.graphicsInfo->paramsBuffers[(size_t)RendererTypes::kStatic]);
	storageBuffers_.push_back(createInfo.graphicsInfo->materialBuffer[(size_t)RendererTypes::kStatic]);
	descriptorSets_.push_back(createInfo.globalRenderData->getSet());
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());

	auto pushConstRange = setupPushConstantRange<TessCtrlPushConstants>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

	DescriptorOffsets offsetConstants;
	offsetConstants.mvp = graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kTerrain];
	offsetConstants.params = graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kTerrain];
	offsetConstants.material = graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kTerrain];
	std::vector<VkSpecializationMapEntry> mapEntry = { 
		makeSpecConstantEntry(0, 0,								 sizeof(offsetConstants.mvp)), 
		makeSpecConstantEntry(1, sizeof(offsetConstants.mvp),	 sizeof(offsetConstants.params)),
		makeSpecConstantEntry(2, sizeof(offsetConstants.params), sizeof(offsetConstants.material))
	};
	auto specConstant = setupSpecConstants(mapEntry, sizeof(DescriptorOffsets), &offsetConstants);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &specConstant);
	stageInfos.emplace_back(createInfo.tessControlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, &specConstant);
	stageInfos.emplace_back(createInfo.tessEvalShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, &specConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Terrain";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<Vertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data(), 1, &pushConstRange), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
}

TerrainRenderer::~TerrainRenderer()
{
}

void TerrainRenderer::createDescriptors(const uint32_t entityCount)
{
}

void TerrainRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buffer()->bind(cmdBuffer, idx);

	const uint32_t dynamicOffsets[3] = {
		graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kStatic] * idx,
		graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kStatic] * idx,
		graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kStatic] * idx
	};

	glm::mat4* eleDataPtr = (glm::mat4*)(static_cast<char*>(storageBuffers_[0]->bindRange()) + dynamicOffsets[0]);
	StaticShaderParams* matDataPtr = (StaticShaderParams*)(static_cast<char*>(storageBuffers_[1]->bindRange()) + dynamicOffsets[1]);
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getModelMatrix();
		glm::mat4 mvp = GraphicsMaster::kProjectionMatrix * camera.viewMatrix * model;
		eleDataPtr[i] = {
			mvp
		};
		matDataPtr[i] = *static_cast<StaticShaderParams*>((*(instPtr + i))->getShaderParams());
		matDataPtr[i].model = model;
	}
	storageBuffers_[1]->unbindRange();
	storageBuffers_[0]->unbindRange();

	uint32_t* dataPtr = (uint32_t*)((char*)storageBuffers_[2]->bindRange() + dynamicOffsets[2]);
	for (size_t i = 0; i < renderStorage_->instanceCount(); i += 3) {
		dataPtr[i] = 2;
		dataPtr[i + 1] = 4;
		dataPtr[i + 2] = 3;
	}
	storageBuffers_[2]->unbindRange();
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		TessCtrlPushConstants pcs;
		pcs.distanceFarMinusClose = 300.0f; // Implies far distance is 350.0f+
		pcs.closeDistance = 50.0f;
		pcs.patchRadius = 40.0f;
		pcs.maxTessellationWeight = 4.0f;
		camera.calculateFrustumPlanes(GraphicsMaster::kProjectionMatrix * camera.viewMatrix, pcs.frustumPlanes);

		VkDescriptorSet sets[2] = { descriptorSets_[0], descriptorSets_[1] };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, sets, 3, dynamicOffsets);

		vkCmdPushConstants(cmdBuffer, pipeline_->getLayout(), pushConstantInfos_[0].stages, pushConstantInfos_[0].offset, pushConstantInfos_[0].size, &pcs);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TerrainRenderer::updateBuffers(LogicalCamera& camera)
{
}
