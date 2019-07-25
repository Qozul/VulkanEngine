#include "ComputeFetchRenderer.h"
#include "ElementBuffer.h"
#include "StorageBuffer.h"
#include "LogicDevice.h"
#include "Descriptor.h"
#include "RendererPipeline.h"
#include "ComputePipeline.h"

using namespace QZL;
using namespace QZL::Graphics;

ComputeFetchRenderer::ComputeFetchRenderer(const LogicDevice* logicDevice, VkRenderPass renderPass, VkExtent2D swapChainExtent, Descriptor* descriptor,
	const std::string& vertexShader, const std::string& fragmentShader, const uint32_t entityCount)
	: RendererBase(logicDevice->getDeviceMemory()), logicDevice_(logicDevice), computePipeline_(nullptr), readbackFence_(VK_NULL_HANDLE)
{
	if (entityCount > 0) {
		StorageBuffer* mvpBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, 0, 0,
			sizeof(ElementData) * entityCount, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);
		storageBuffers_.push_back(mvpBuf);

		StorageBuffer * transformBuf = new StorageBuffer(logicDevice, MemoryAllocationPattern::kDynamicResource, 2, 0,
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

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		CHECK_VKRESULT(vkCreateFence(*logicDevice_, &fenceInfo, nullptr, &readbackFence_));
	}
	else {
		createPipeline(logicDevice, renderPass, swapChainExtent, RendererPipeline::makeLayoutInfo(0, nullptr), vertexShader, fragmentShader);
	}
}

ComputeFetchRenderer::~ComputeFetchRenderer()
{
	if (readbackFence_ != VK_NULL_HANDLE)
		vkDestroyFence(*logicDevice_, readbackFence_, nullptr);
	SAFE_DELETE(computePipeline_);
}

void ComputeFetchRenderer::initialise(const glm::mat4& viewMatrix)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	Transform* data = static_cast<Transform*>(storageBuffers_[1]->bindRange());
	memcpy(data, renderStorage_->instanceData(), renderStorage_->instanceCount() * sizeof(Transform));
	storageBuffers_[1]->unbindRange();
}

void ComputeFetchRenderer::recordCompute(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
{
	if (renderStorage_->instanceCount() == 0)
		return;
	// No longer need pipeline barriers because the compute shader is submitted and waited for before doing graphics rendering
	std::array<glm::mat4, 2U> pushConstantValue = { viewMatrix, GraphicsMaster::kProjectionMatrix };
	vkCmdPushConstants(cmdBuffer, computePipeline_->getLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::mat4) * 2, pushConstantValue.data());
	vkCmdPipelineBarrier(cmdBuffer, VK_SHADER_STAGE_COMPUTE_BIT, VK_SHADER_STAGE_COMPUTE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 1, &pushConstantBarrier_, 0, nullptr, 0, nullptr);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline_->getPipeline());
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline_->getLayout(), 0, 1, &descriptorSets_[idx], 0, nullptr);
	vkCmdDispatch(cmdBuffer, renderStorage_->instanceCount(), 1, 1);

	CHECK_VKRESULT(vkEndCommandBuffer(cmdBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	vkResetFences(*logicDevice_, 1, &readbackFence_);
	CHECK_VKRESULT(vkQueueSubmit(logicDevice_->getQueueHandle(QueueFamilyType::kGraphicsQueue), 1, &submitInfo, readbackFence_));
	vkWaitForFences(*logicDevice_, 1, &readbackFence_, VK_TRUE, std::numeric_limits<uint64_t>::max());
	readback();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	CHECK_VKRESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
}

void ComputeFetchRenderer::readback()
{
	Transform* data = static_cast<Transform*>(storageBuffers_[1]->bindRange());
	memcpy(renderStorage_->instanceData(), data, renderStorage_->instanceCount() * sizeof(Transform));
	storageBuffers_[1]->unbindRange();
}

void ComputeFetchRenderer::recordFrame(const glm::mat4& viewMatrix, const uint32_t idx, VkCommandBuffer cmdBuffer)
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
