#include "ComputeRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "RendererPipeline.h"
#include "ComputePipeline.h"
#include "GraphicsComponent.h"
#include "../Assets/Entity.h"

using namespace QZL;
using namespace QZL::Graphics;

ComputeRenderer::ComputeRenderer(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount)
	: RendererBase(logicDevice->getDeviceMemory())
{
	if (entityCount > 0) {
		StorageBuffer* mvpBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
			sizeof(ElementData) * entityCount, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);
		storageBuffers_.push_back(mvpBuf);

		StorageBuffer* transformBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, 2, 0,
			sizeof(Transform) * entityCount, VK_SHADER_STAGE_COMPUTE_BIT);
		storageBuffers_.push_back(transformBuf);

		auto computeLayout = descriptor->makeLayout({ mvpBuf->getBinding(), transformBuf->getBinding() });
		size_t idx = descriptor->createSets({ computeLayout, computeLayout, computeLayout });
		std::vector<VkWriteDescriptorSet> descWrites;
		for (int i = 0; i < 3; ++i) {
			descriptorSets_.push_back(descriptor->getSet(idx + i));
			descWrites.push_back(mvpBuf->descriptorWrite(descriptor->getSet(idx + i)));
			descWrites.push_back(transformBuf->descriptorWrite(descriptor->getSet(idx + i)));
		}
		descriptor->updateDescriptorSets(descWrites);

		createPipeline(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(1, &computeLayout), vertexShader, fragmentShader);
		computePipeline_ = new ComputePipeline(logicDevice, ComputePipeline::makeLayoutInfo(1, &computeLayout), "Compute");

		pushConstantBarrier_ = {};
		pushConstantBarrier_.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		pushConstantBarrier_.pNext = NULL;

		bufMemoryBarrier_ = {};
		bufMemoryBarrier_.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufMemoryBarrier_.pNext = NULL;
		bufMemoryBarrier_.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		bufMemoryBarrier_.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufMemoryBarrier_.buffer = mvpBuf->getBufferDetails().buffer;
		bufMemoryBarrier_.srcQueueFamilyIndex = logicDevice->getFamilyIndex(QueueFamilyType::kGraphicsQueue);
		bufMemoryBarrier_.dstQueueFamilyIndex = logicDevice->getFamilyIndex(QueueFamilyType::kGraphicsQueue);
		bufMemoryBarrier_.offset = 0;
		bufMemoryBarrier_.size = VK_WHOLE_SIZE;

		bufMemoryBarrier2_ = {};
		bufMemoryBarrier2_.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufMemoryBarrier2_.pNext = NULL;
		bufMemoryBarrier2_.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufMemoryBarrier2_.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		bufMemoryBarrier2_.buffer = mvpBuf->getBufferDetails().buffer;
		bufMemoryBarrier2_.srcQueueFamilyIndex = logicDevice->getFamilyIndex(QueueFamilyType::kGraphicsQueue);
		bufMemoryBarrier2_.dstQueueFamilyIndex = logicDevice->getFamilyIndex(QueueFamilyType::kGraphicsQueue);
		bufMemoryBarrier2_.offset = 0;
		bufMemoryBarrier2_.size = VK_WHOLE_SIZE;

		transBufMemoryBarrier_ = {};
		transBufMemoryBarrier_.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		transBufMemoryBarrier_.pNext = NULL;
		transBufMemoryBarrier_.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		transBufMemoryBarrier_.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		transBufMemoryBarrier_.buffer = transformBuf->getBufferDetails().buffer;
		transBufMemoryBarrier_.srcQueueFamilyIndex = logicDevice->getFamilyIndex(QueueFamilyType::kGraphicsQueue);
		transBufMemoryBarrier_.dstQueueFamilyIndex = logicDevice->getFamilyIndex(QueueFamilyType::kGraphicsQueue);
		transBufMemoryBarrier_.offset = 0;
		transBufMemoryBarrier_.size = VK_WHOLE_SIZE;
	}
	else {
		createPipeline(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(0, nullptr), vertexShader, fragmentShader);
	}
}

ComputeRenderer::~ComputeRenderer()
{
	SAFE_DELETE(computePipeline_);
}

void ComputeRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	Transform* data = static_cast<Transform*>(storageBuffers_[1]->bindRange());
	auto instPtr = renderStorage_->instanceData();
	for (size_t i = 0; i < renderStorage_->instanceCount(); ++i) {
		data[i] = *(*(instPtr + i))->getEntity()->getTransform();
	}
	storageBuffers_[1]->unbindRange();
}

void ComputeRenderer::recordCompute(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_COMPUTE_BIT, 0, 0, nullptr, 1, &bufMemoryBarrier2_, 0, nullptr);
	vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_COMPUTE_BIT, 0, 0, nullptr, 1, &transBufMemoryBarrier_, 0, nullptr);

	std::array<glm::mat4, 2U> pushConstantValue = { viewMatrix, GraphicsMaster::kProjectionMatrix };
	vkCmdPushConstants(cmdBuffer, computePipeline_->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::mat4) * 2, pushConstantValue.data());
	vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_COMPUTE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantBarrier_, 0, nullptr, 0, nullptr);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline_->getPipeline());
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline_->getLayout(), 0, 1, &descriptorSets_[idx], 0, nullptr);
	vkCmdDispatch(cmdBuffer, renderStorage_->instanceCount(), 1, 1);

	vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_VERTEX_BIT, 0, 0, nullptr, 1, &bufMemoryBarrier_, 0, nullptr);
}

void ComputeRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	beginFrame(cmdBuffer);

	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getLayout(), 0, 1, &descriptorSets_[idx], 0, nullptr);
	renderStorage_->buf()->bind(cmdBuffer);
	for (int i = 0; i < renderStorage_->meshCount(); ++i) {
		const DrawElementsCommand& drawElementCmd = renderStorage_->meshData()[i];
		vkCmdDrawIndexed(cmdBuffer, drawElementCmd.indexCount, drawElementCmd.instanceCount, drawElementCmd.firstIndex, drawElementCmd.baseVertex, drawElementCmd.baseInstance);
	}
}
