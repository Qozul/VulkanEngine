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

TerrainRenderer::TerrainRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), 
		sizeof(uint16_t)), RenderStorage::InstanceUsage::kUnlimited))
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());
	storageBuffers_.push_back(createInfo.graphicsInfo->mvpBuffer);
	storageBuffers_.push_back(createInfo.graphicsInfo->paramsBuffer);
	storageBuffers_.push_back(createInfo.graphicsInfo->materialBuffer);

	VkPushConstantRange pushConstants[2] = {
		setupPushConstantRange<CameraPushConstants>(VK_SHADER_STAGE_VERTEX_BIT),
		setupPushConstantRange<TessellationPushConstants>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
	};

	uint32_t offsets[3] = { graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kTerrain], graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kTerrain], graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kTerrain] };
	std::vector<VkSpecializationMapEntry> mapEntry = {
		makeSpecConstantEntry(0, 0,	sizeof(uint32_t)),
		makeSpecConstantEntry(1, sizeof(uint32_t), sizeof(uint32_t)),
		makeSpecConstantEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t))
	};
	auto tescSpecConstant = setupSpecConstants(1, mapEntry.data(), sizeof(uint32_t), &offsets[2]);
	auto teseSpecConstant = setupSpecConstants(3, mapEntry.data(), sizeof(uint32_t) * 3, offsets);
	auto fragSpecConstant = setupSpecConstants(2, mapEntry.data(), sizeof(uint32_t) * 2, &offsets[1]);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, &fragSpecConstant);
	stageInfos.emplace_back(createInfo.tessControlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, &tescSpecConstant);
	stageInfos.emplace_back(createInfo.tessEvalShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, &teseSpecConstant);

	PipelineCreateInfo pci = {};
	pci.debugName = "Terrain";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<Vertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()), 
		pipelineLayouts_.data(), 2, pushConstants), stageInfos, pci, RendererPipeline::PrimitiveType::kQuads);
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
		graphicsInfo_->mvpRange * idx,
		graphicsInfo_->paramsRange * idx,
		graphicsInfo_->materialRange * idx
	};

	glm::mat4* eleDataPtr = (glm::mat4*)(static_cast<char*>(storageBuffers_[0]->bindRange()) + sizeof(glm::mat4) * graphicsInfo_->mvpOffsetSizes[(size_t)RendererTypes::kTerrain] + dynamicOffsets[0]);
	StaticShaderParams* matDataPtr = (StaticShaderParams*)(static_cast<char*>(storageBuffers_[1]->bindRange()) + 
		sizeof(StaticShaderParams) * graphicsInfo_->paramsOffsetSizes[(size_t)RendererTypes::kTerrain] + dynamicOffsets[1]);
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

	uint32_t* dataPtr = (uint32_t*)((char*)storageBuffers_[2]->bindRange() + sizeof(Materials::Terrain) * graphicsInfo_->materialOffsetSizes[(size_t)RendererTypes::kTerrain] + dynamicOffsets[2]);
	for (size_t i = 0; i < renderStorage_->instanceCount(); i += 3) {
		dataPtr[i] = 2;
		dataPtr[i + 1] = 4;
		dataPtr[i + 2] = 3;
	}
	storageBuffers_[2]->unbindRange();
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];


		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TerrainRenderer::updateBuffers(LogicalCamera& camera)
{
}
