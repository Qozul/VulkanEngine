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

struct TessCtrlPushConstants {
	float distanceFarMinusClose = 0.0f;
	float closeDistance = 0.0f;
	float patchRadius = 0.0f;
	float maxTessellationWeight = 0.0f;
	std::array<glm::vec4, 6> frustumPlanes;
};

TexturedRenderer::TexturedRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), sizeof(uint16_t)), 
		RenderStorage::InstanceUsage::kUnlimited))
{
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layout);
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());
	storageBuffers_.push_back(createInfo.graphicsInfo->mvpBuffer);
	storageBuffers_.push_back(createInfo.graphicsInfo->paramsBuffer);
	storageBuffers_.push_back(createInfo.graphicsInfo->materialBuffer);

	auto pushConstRange = setupPushConstantRange<TessCtrlPushConstants>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

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
		pipelineLayouts_.data(), 1, &pushConstRange), stageInfos, pci);
}

TexturedRenderer::~TexturedRenderer()
{
}

void TexturedRenderer::createDescriptors(const uint32_t entityCount)
{
}

void TexturedRenderer::recordFrame(LogicalCamera& camera, const uint32_t idx, VkCommandBuffer cmdBuffer)
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
	
	glm::mat4* eleDataPtr = (glm::mat4*)(static_cast<char*>(storageBuffers_[0]->bindRange()) + dynamicOffsets[0]);
	StaticShaderParams* paramsPtr = (StaticShaderParams*)(static_cast<char*>(storageBuffers_[1]->bindRange()) + dynamicOffsets[1]);
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getModelMatrix();
		eleDataPtr[i] = {
			GraphicsMaster::kProjectionMatrix * camera.viewMatrix * model
		};
		paramsPtr[i] = {
			*static_cast<StaticShaderParams*>((*(instPtr + i))->getShaderParams())
		};
		paramsPtr[i].model = model;
	}
	storageBuffers_[1]->unbindRange();
	storageBuffers_[0]->unbindRange();
	uint32_t* dataPtr = (uint32_t*)((char*)storageBuffers_[2]->bindRange() + 2 * sizeof(Materials::Static) + dynamicOffsets[2]);
	for (size_t i = 0; i < renderStorage_->instanceCount(); i++) {
		dataPtr[i] = 0;
		dataPtr[i + 1] = 1;
	}
	storageBuffers_[2]->unbindRange();

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TexturedRenderer::recordDIFrame(const uint32_t idx, VkCommandBuffer cmdBuffer)
{
}

void TexturedRenderer::updateBuffers(const glm::mat4& viewMatrix)
{
}

void TexturedRenderer::updateDIBuffer()
{
}
