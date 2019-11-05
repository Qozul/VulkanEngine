// Author: Ralph Ridley
// Date: 03/11/19
#include "DynamicElementBuffer.h"
#include "DeviceMemory.h"

using namespace QZL;
using namespace QZL::Graphics;

DynamicElementBuffer::DynamicElementBuffer(DeviceMemory* deviceMemory, size_t swapChainImageCount, size_t sizeOfVertices, size_t sizeOfIndices)
	: ElementBufferObject(deviceMemory, sizeOfVertices, sizeOfIndices), swapChainImageCount_(swapChainImageCount)
{
	isDynamic_ = true;
}

void DynamicElementBuffer::commit()
{
	vertexBufferDetails_ = deviceMemory_->createBuffer("DynamicEBO VertexBuffer", MemoryAllocationPattern::kDynamicResource, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexData_.size() * swapChainImageCount_);
	isCommitted_ = true;
}

void DynamicElementBuffer::bind(VkCommandBuffer cmdBuffer, const size_t idx)
{
	VkDeviceSize offset = vertexData_.size() * idx;
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBufferDetails_.buffer, &offset);
	if (isIndexed()) {
		vkCmdBindIndexBuffer(cmdBuffer, getIndexBuffer(), 0, indexType_);
	}
}

void DynamicElementBuffer::updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx)
{
	if (vertexBufferDetails_.mappedData != nullptr) {
		memcpy(static_cast<char*>(vertexBufferDetails_.mappedData) + (idx * vertexData_.size()), vertexData_.data(), vertexData_.size());
	}
	else {
		char* dataPtr = static_cast<char*>(deviceMemory_->mapMemory(vertexBufferDetails_.id));
		memcpy(dataPtr + (idx * vertexData_.size()), vertexData_.data(), vertexData_.size());
		deviceMemory_->unmapMemory(vertexBufferDetails_.id);
	}
}

SubBufferRange DynamicElementBuffer::allocateSubBufferRange(size_t count)
{
	return { addVertices(nullptr, count * sizeOfVertices_), count };
}

void* DynamicElementBuffer::getSubBufferData(size_t firstVertex)
{
	return &vertexData_[firstVertex];
}
