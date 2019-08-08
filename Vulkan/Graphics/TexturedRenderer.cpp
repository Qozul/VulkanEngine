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
#include "StaticShaderParams.h"
#include "StaticRenderStorage.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

TexturedRenderer::TexturedRenderer(LogicDevice* logicDevice, TextureManager* textureManager, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData)
	: RendererBase(logicDevice), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);
	if (logicDevice->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
		renderStorage_ = new RenderStorage(new ElementBuffer<Vertex>(logicDevice->getDeviceMemory()));
	}
	else {
		renderStorage_ = new StaticRenderStorage(textureManager, logicDevice, new ElementBuffer<Vertex>(logicDevice->getDeviceMemory()));
	}

	StorageBuffer* mvpBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)ReservedGraphicsBindings0::PER_ENTITY_DATA, 0,
		sizeof(ElementData) * entityCount, VK_SHADER_STAGE_VERTEX_BIT);
	StorageBuffer* matBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)ReservedGraphicsBindings0::MATERIAL_DATA, 0,
		sizeof(MaterialStatic) * entityCount, VK_SHADER_STAGE_FRAGMENT_BIT);
	storageBuffers_.push_back(mvpBuf);
	storageBuffers_.push_back(matBuf);

	VkDescriptorSetLayout layout;
	if (!logicDevice->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
		VkDescriptorSetLayoutBinding diffuseBinding = {};
		diffuseBinding.binding = (uint32_t)ReservedGraphicsBindings0::TEXTURE_0;
		diffuseBinding.descriptorCount = 1;
		diffuseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		diffuseBinding.pImmutableSamplers = nullptr;
		diffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding normalMapBinding = {};
		normalMapBinding.binding = (uint32_t)ReservedGraphicsBindings0::TEXTURE_1;
		normalMapBinding.descriptorCount = 1;
		normalMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalMapBinding.pImmutableSamplers = nullptr;
		normalMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layout = descriptor->makeLayout({ mvpBuf->getBinding(), matBuf->getBinding(), diffuseBinding, normalMapBinding });
	}
	else {
		layout = descriptor->makeLayout({ mvpBuf->getBinding(), matBuf->getBinding() });
	}

	pipelineLayouts_.push_back(layout);
	pipelineLayouts_.push_back(globalRenderData->layout);

	size_t idx = descriptor->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor->getSet(idx + i));
		descriptorSets_.push_back(globalRenderData->globalDataDescriptor->getSet(globalRenderData->setIdx));
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor->getSet(idx + i)));
		descWrites.push_back(matBuf->descriptorWrite(descriptor->getSet(idx + i)));
	}
	descriptor->updateDescriptorSets(descWrites);

	createPipeline(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(pipelineLayouts_.size(), pipelineLayouts_.data()), vertexShader, fragmentShader);
}

TexturedRenderer::~TexturedRenderer()
{
}

void TexturedRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	MaterialStatic* matDataPtr = static_cast<MaterialStatic*>(storageBuffers_[1]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		matDataPtr[i] = static_cast<StaticShaderParams*>((*(instPtr + i))->getShaderParams())->getMaterial();
	}
	storageBuffers_[1]->unbindRange();
}

void TexturedRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);
	renderStorage_->buf()->bind(cmdBuffer);

	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getTransform()->toModelMatrix();
		eleDataPtr[i] = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model
		};
	}
	storageBuffers_[0]->unbindRange();

	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];

		auto srs = static_cast<StaticRenderStorage*>(renderStorage_);
		if (!logicDevice_->supportsOptionalExtension(OptionalExtensions::DESCRIPTOR_INDEXING)) {
			std::vector<VkWriteDescriptorSet> descWrites;
			descWrites.push_back(srs->getParamData(i).diffuse->descriptorWrite(descriptorSets_[idx * 2]));
			descWrites.push_back(srs->getParamData(i).normalMap->descriptorWrite(descriptorSets_[idx * 2]));
			descriptor_->updateDescriptorSets(descWrites);
		}
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, &descriptorSets_[idx * 2], 0, nullptr);
		
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.indexCount, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
