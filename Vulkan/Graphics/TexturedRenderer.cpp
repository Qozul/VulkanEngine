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
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

TexturedRenderer::TexturedRenderer(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount)
	: RendererBase(logicDevice->getDeviceMemory()), descriptor_(descriptor)
{
	if (entityCount > 0) {
		StorageBuffer* mvpBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
			sizeof(ElementData) * entityCount, VK_SHADER_STAGE_VERTEX_BIT);

		VkDescriptorSetLayoutBinding diffuseBinding = {};
		diffuseBinding.binding = 1;
		diffuseBinding.descriptorCount = 1;
		diffuseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		diffuseBinding.pImmutableSamplers = nullptr;
		diffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding normalMapBinding = {};
		normalMapBinding.binding = 3;
		normalMapBinding.descriptorCount = 1;
		normalMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalMapBinding.pImmutableSamplers = nullptr;
		normalMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		auto layout = descriptor->makeLayout({ mvpBuf->getBinding(), diffuseBinding, normalMapBinding });
		storageBuffers_.push_back(mvpBuf);
		size_t idx = descriptor->createSets({ layout, layout, layout });
		std::vector<VkWriteDescriptorSet> descWrites;
		for (int i = 0; i < 3; ++i) {
			descriptorSets_.push_back(descriptor->getSet(idx + i));
			descWrites.push_back(mvpBuf->descriptorWrite(descriptor->getSet(idx + i)));
		}
		descriptor->updateDescriptorSets(descWrites);

		createPipeline(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(storageBuffers_.size(), &layout), vertexShader, fragmentShader);
	}
	else {
		createPipeline(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(0, nullptr), vertexShader, fragmentShader);
	}
}

TexturedRenderer::~TexturedRenderer()
{
}

void TexturedRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	if (Shared::kProjectionMatrix[1][1] >= 0)
		Shared::kProjectionMatrix[1][1] *= -1;
	ElementData* eleDataPtr = static_cast<ElementData*>(storageBuffers_[0]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		glm::mat4 model = (*(instPtr + i))->getEntity()->getTransform()->toModelMatrix();
		eleDataPtr[i] = {
			model, Shared::kProjectionMatrix * viewMatrix * model
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
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		std::vector<VkWriteDescriptorSet> descWrites;
		const StaticShaderParams* params = (*renderStorage_->instanceData())->getShaderParams().ssp;
		descWrites.push_back(params->diffuse_->descriptorWrite(descriptorSets_[idx]));
		descWrites.push_back(params->normalMap_->descriptorWrite(descriptorSets_[idx]));
		descriptor_->updateDescriptorSets(descWrites);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, &descriptorSets_[idx], 0, nullptr);

		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.indexCount, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
