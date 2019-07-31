#include "TexturedRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "TextureSampler.h"
#include "TextureLoader.h"
#include "DeviceMemory.h"
#include "RendererPipeline.h"
#include "GraphicsComponent.h"
#include "StaticShaderParams.h"
#include "StaticRenderStorage.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

TexturedRenderer::TexturedRenderer(const LogicDevice* logicDevice, TextureLoader*& textureLoader, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount, const GlobalRenderData* globalRenderData)
	: RendererBase(new StaticRenderStorage(textureLoader, logicDevice)), descriptor_(descriptor)
{
	ASSERT(entityCount > 0);
	StorageBuffer* mvpBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, (uint32_t)ReservedGraphicsBindings::PER_ENTITY_DATA, 0,
		sizeof(ElementData) * entityCount, VK_SHADER_STAGE_VERTEX_BIT);
	storageBuffers_.push_back(mvpBuf);

	VkDescriptorSetLayoutBinding diffuseBinding = {};
	diffuseBinding.binding = (uint32_t)ReservedGraphicsBindings::TEXTURE_0;
	diffuseBinding.descriptorCount = 1;
	diffuseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	diffuseBinding.pImmutableSamplers = nullptr;
	diffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding normalMapBinding = {};
	normalMapBinding.binding = (uint32_t)ReservedGraphicsBindings::TEXTURE_1;
	normalMapBinding.descriptorCount = 1;
	normalMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalMapBinding.pImmutableSamplers = nullptr;
	normalMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	auto layout = descriptor->makeLayout({ mvpBuf->getBinding(), diffuseBinding, normalMapBinding });
	pipelineLayouts_.push_back(layout);
	pipelineLayouts_.push_back(globalRenderData->lightingDataLayout);

	size_t idx = descriptor->createSets({ layout, layout, layout });
	std::vector<VkWriteDescriptorSet> descWrites;
	for (int i = 0; i < 3; ++i) {
		descriptorSets_.push_back(descriptor->getSet(idx + i));
		descriptorSets_.push_back(globalRenderData->globalDataDescriptor->getSet(globalRenderData->lightingDataSetsIdx + i));
		descWrites.push_back(mvpBuf->descriptorWrite(descriptor->getSet(idx + i)));
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
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getTransform()->toModelMatrix();
		eleDataPtr[i] = {
			model, GraphicsMaster::kProjectionMatrix * viewMatrix * model
		};
	}
	storageBuffers_[0]->unbindRange();
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
		std::vector<VkWriteDescriptorSet> descWrites;
		descWrites.push_back(srs->getParamData(i).diffuse->descriptorWrite(descriptorSets_[idx * 2]));
		descWrites.push_back(srs->getParamData(i).normalMap->descriptorWrite(descriptorSets_[idx * 2]));
		descriptor_->updateDescriptorSets(descWrites);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 2, &descriptorSets_[idx * 2], 0, nullptr);

		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.indexCount, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
