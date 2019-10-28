#include "TexturedRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "TextureManager.h"
#include "DeviceMemory.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "ShaderParams.h"
#include "Material.h"
#include "RenderObject.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

TexturedRenderer::TexturedRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData)
	: RendererBase(logicDevice, new RenderStorage(new ElementBuffer<Vertex>(logicDevice->getDeviceMemory()), RenderStorage::InstanceUsage::UNLIMITED)), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);

	descriptorSets_.push_back(globalRenderData->getSet());
	pipelineLayouts_.push_back(globalRenderData->getLayout());
	createDescriptors(entityCount);

	std::vector<ShaderStageInfo> stageInfos;
	stageInfos.emplace_back(vertexShader, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
	stageInfos.emplace_back(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	PipelineCreateInfo pci = {};
	pci.enableDepthTest = VK_TRUE;
	pci.enableDepthWrite = VK_TRUE;
	pci.extent = swapChainExtent;
	pci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pci.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	createPipeline<Vertex>(logicDevice, renderPass, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data()), stageInfos, pci);
}

TexturedRenderer::~TexturedRenderer()
{
}

void TexturedRenderer::createDescriptors(const uint32_t entityCount)
{
	DescriptorBuffer* mvpBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 0, 0,
		sizeof(ElementData) * entityCount, VK_SHADER_STAGE_VERTEX_BIT);
	DescriptorBuffer* paramsBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 1, 0,
		sizeof(StaticShaderParams::Params) * entityCount, VK_SHADER_STAGE_FRAGMENT_BIT);
	storageBuffers_.push_back(mvpBuf);
	storageBuffers_.push_back(paramsBuf);

	VkDescriptorSetLayout layout;
	DescriptorBuffer* diBuf = nullptr;
	if (logicDevice_->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
		diBuf = DescriptorBuffer::makeBuffer<StorageBuffer>(logicDevice_, MemoryAllocationPattern::kDynamicResource, 2, 0,
			sizeof(uint32_t) * 2 * entityCount, VK_SHADER_STAGE_FRAGMENT_BIT);
		storageBuffers_.push_back(diBuf);
		layout = descriptor_->makeLayout({ mvpBuf->getBinding(), paramsBuf->getBinding(), diBuf->getBinding() });
	}
	else {
		layout = descriptor_->makeLayout({ mvpBuf->getBinding(), paramsBuf->getBinding() });
	}

	pipelineLayouts_.push_back(layout);
	if (!logicDevice_->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
		pipelineLayouts_.push_back(StaticMaterial::getLayout(descriptor_));
	}

	size_t setIdx = descriptor_->createSets({ layout, layout, layout });

	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor_->getSet(setIdx + i));
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor_->getSet(setIdx + i)));
		descWrites.push_back(paramsBuf->descriptorWrite(descriptor_->getSet(setIdx + i)));
		if (diBuf != nullptr) {
			descWrites.push_back(diBuf->descriptorWrite(descriptor_->getSet(setIdx + i)));
		}
	}
	descriptor_->updateDescriptorSets(descWrites);
}

void TexturedRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	static_cast<ElementBufferInterface*>(renderStorage_->buf())->bind(cmdBuffer, idx);

	updateBuffers(viewMatrix);

	if (logicDevice_->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
		recordDIFrame(viewMatrix, idx, cmdBuffer);
	}
	else {
		recordNormalFrame(viewMatrix, idx, cmdBuffer);
	}
}

void TexturedRenderer::recordDIFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	// Textures defined per instance using descriptor indexing
	updateDIBuffer();
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		VkDescriptorSet sets[2] = { descriptorSets_[1 + idx], descriptorSets_[0] };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, sets, 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TexturedRenderer::recordNormalFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	// Texture defined per mesh, not per instance
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		RenderObject* robject = renderStorage_->renderObjectData()[i];

		VkDescriptorSet sets[3] = { descriptorSets_[0], descriptorSets_[1 + idx], robject->getMaterial()->getTextureSet() };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 3, sets, 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.count, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}

void TexturedRenderer::updateBuffers(const glm::mat4& viewMatrix)
{
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	StaticShaderParams::Params* paramsPtr = static_cast<StaticShaderParams::Params*>(storageBuffers_[1]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getTransform()->toModelMatrix();
		eleDataPtr[i] = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model
		};
		paramsPtr[i] = {
			static_cast<StaticShaderParams*>((*(instPtr + i))->getShaderParams())->params
		};
	}
	storageBuffers_[1]->unbindRange();
	storageBuffers_[0]->unbindRange();
}

void TexturedRenderer::updateDIBuffer()
{
	uint32_t* dataPtr = static_cast<uint32_t*>(storageBuffers_[2]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); i += 2) {
		dataPtr[i] = static_cast<StaticMaterial*>((*(instPtr + i))->getMaterial())->diffuse_.diffuseTextureIndex;
		dataPtr[i + 1] = static_cast<StaticMaterial*>((*(instPtr + i))->getMaterial())->normalMap_.normalMapIndex;
	}
	storageBuffers_[2]->unbindRange();
}
