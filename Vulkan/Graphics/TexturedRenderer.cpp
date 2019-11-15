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

TexturedRenderer::TexturedRenderer(RendererCreateInfo& createInfo)
	: RendererBase(createInfo, new RenderStorage(new ElementBufferObject(createInfo.logicDevice->getDeviceMemory(), sizeof(Vertex), sizeof(uint16_t)), 
		RenderStorage::InstanceUsage::kUnlimited))
{
	descriptorSets_.push_back(createInfo.globalRenderData->getSet());
	pipelineLayouts_.push_back(createInfo.globalRenderData->getLayout());
	pipelineLayouts_.push_back(createInfo.graphicsInfo->layouts[(size_t)RendererTypes::kStatic]);
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor_->getSet(createInfo.graphicsInfo->sets[(size_t)RendererTypes::kStatic] + i));
	}
	storageBuffers_.push_back(createInfo.graphicsInfo->mvpBuffer[(size_t)RendererTypes::kStatic]);
	storageBuffers_.push_back(createInfo.graphicsInfo->paramsBuffers[(size_t)RendererTypes::kStatic]);
	storageBuffers_.push_back(createInfo.graphicsInfo->materialBuffer[(size_t)RendererTypes::kStatic]);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(createInfo.vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(createInfo.fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.debugName = "Static";
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = createInfo.extent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pci.subpassIndex = createInfo.subpassIndex;

	createPipeline<Vertex>(createInfo.logicDevice, createInfo.renderPass, RendererPipeline::makeLayoutInfo(static_cast<uint32_t>(pipelineLayouts_.size()),
		pipelineLayouts_.data()), stageInfos, pci);
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

	updateBuffers(camera.viewMatrix);

	recordDIFrame(idx, cmdBuffer);
}

void TexturedRenderer::recordDIFrame(const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	updateDIBuffer();
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		VkDescriptorSet sets[2] = { descriptorSets_[0], descriptorSets_[1 + (size_t)idx] };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, sets, 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TexturedRenderer::updateBuffers(const glm::mat4& viewMatrix)
{
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	StaticShaderParams* paramsPtr = static_cast<StaticShaderParams*>(storageBuffers_[1]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getModelMatrix();
		eleDataPtr[i] = {
			GraphicsMaster::kProjectionMatrix * viewMatrix * model
		};
		paramsPtr[i] = {
			*static_cast<StaticShaderParams*>((*(instPtr + i))->getShaderParams())
		};
		paramsPtr[i].model = model;
	}
	storageBuffers_[1]->unbindRange();
	storageBuffers_[0]->unbindRange();
}

void TexturedRenderer::updateDIBuffer()
{
	uint32_t* dataPtr = (uint32_t*)storageBuffers_[2]->bindRange();
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); i++) {
		dataPtr[i] = 0;
		dataPtr[i + 1] = 1;
	}
	storageBuffers_[2]->unbindRange();
}
