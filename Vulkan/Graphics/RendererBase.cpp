#include "RendererBase.h"
#include "RenderStorage.h"
#include "ElementBufferObject.h"
#include "GraphicsComponent.h"
#include "StorageBuffer.h"
#include "Mesh.h"

using namespace QZL;
using namespace QZL::Graphics;

RendererBase::~RendererBase()
{
	for (auto& buffer : storageBuffers_) {
		SAFE_DELETE(buffer);
	}
	SAFE_DELETE(renderStorage_);
	SAFE_DELETE(pipeline_);
}

std::vector<VkWriteDescriptorSet> RendererBase::getDescriptorWrites(uint32_t frameIdx)
{
	std::vector<VkWriteDescriptorSet> writes;
	for (auto& buf : storageBuffers_) {
		writes.push_back(buf->descriptorWrite(descriptorSets_[frameIdx]));
	}
	return writes;
}

void RendererBase::registerComponent(GraphicsComponent* component, RenderObject* robject)
{
	renderStorage_->addMesh(component, robject);
}

ElementBufferObject* RendererBase::getElementBuffer()
{
	return renderStorage_->buffer();
}

VkPipelineLayout RendererBase::getPipelineLayout()
{
	return pipeline_->getLayout();
}

void RendererBase::preframeSetup()
{
	if (renderStorage_ != nullptr) {
		renderStorage_->buffer()->commit();
	}
}

void RendererBase::toggleWiremeshMode()
{
	pipeline_->switchMode();
}

VkSpecializationMapEntry RendererBase::makeSpecConstantEntry(uint32_t id, uint32_t offset, size_t size)
{
	return { id, offset, size };
}

VkSpecializationInfo RendererBase::setupSpecConstants(uint32_t entryCount, VkSpecializationMapEntry* entryPtr, size_t dataSize, const void* data)
{
	return { entryCount, entryPtr, dataSize, data };
}

void RendererBase::beginFrame(VkCommandBuffer& cmdBuffer)
{
	EXPECTS(pipeline_ != nullptr);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->getPipeline());
}

void RendererBase::bindEBO(VkCommandBuffer& cmdBuffer, uint32_t idx)
{
	renderStorage_->buffer()->bind(cmdBuffer, idx);
}

void RendererBase::createPipeline(const LogicDevice* logicDevice, VkRenderPass renderPass, VkPipelineLayoutCreateInfo layoutInfo, std::vector<ShaderStageInfo>& stages,
	PipelineCreateInfo pipelineCreateInfo, RendererPipeline::PrimitiveType patchVertexCount)
{
	pipelineCreateInfo.vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0, 0, nullptr, 0, nullptr };
	pipeline_ = new RendererPipeline(logicDevice, renderPass, layoutInfo, stages, pipelineCreateInfo, patchVertexCount);
}
