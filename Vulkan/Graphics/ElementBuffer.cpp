#include "ElementBuffer.h"
#include "DeviceMemory.h"

using namespace QZL;
using namespace QZL::Graphics;

ElementBuffer::ElementBuffer(DeviceMemory* deviceMemory)
	: deviceMemory_(deviceMemory), isCommitted_(false), indexCount_(0)
{
}

ElementBuffer::~ElementBuffer()
{
	if (isCommitted_) {
		deviceMemory_->deleteAllocation(vertexBufferDetails_.id, vertexBufferDetails_.buffer);
		deviceMemory_->deleteAllocation(indexBufferDetails_.id, indexBufferDetails_.buffer);
	}
}

void ElementBuffer::commit()
{
	if (isCommitted_)
		return;

	size_t size = indices_.size() * sizeof(uint16_t);
	size_t size2 = vertices_.size() * sizeof(Vertex);
	size_t largestSize = size > size2 ? size : size2;
	MemoryAllocationDetails stagingBuffer = deviceMemory_->createBuffer(MemoryAllocationPattern::kStaging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, largestSize);

	void* data = deviceMemory_->mapMemory(stagingBuffer.id);
	memcpy(data, vertices_.data(), size2);
	deviceMemory_->unmapMemory(stagingBuffer.id);

	vertexBufferDetails_ = deviceMemory_->createBuffer(MemoryAllocationPattern::kStaticResource, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size2);

	deviceMemory_->transferMemory(stagingBuffer.buffer, vertexBufferDetails_.buffer, 0, 0, size2);

	data = deviceMemory_->mapMemory(stagingBuffer.id);
	memcpy(data, indices_.data(), size);
	deviceMemory_->unmapMemory(stagingBuffer.id);

	indexBufferDetails_ = deviceMemory_->createBuffer(MemoryAllocationPattern::kStaticResource, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);

	deviceMemory_->transferMemory(stagingBuffer.buffer, indexBufferDetails_.buffer, 0, 0, size);

	deviceMemory_->deleteAllocation(stagingBuffer.id, stagingBuffer.buffer);

	indexCount_ = indices_.size();
	indices_.clear();
	vertices_.clear();
	isCommitted_ = true;
}

const bool ElementBuffer::isCommitted()
{
	return isCommitted_;
}

size_t ElementBuffer::addVertices(Vertex* data, const size_t size)
{
	ASSERT(!isCommitted_);
	const size_t prevSize = vertices_.size();
	vertices_.resize(prevSize + size);
	std::copy_n(data, size, vertices_.begin() + prevSize);
	return prevSize;
}

size_t ElementBuffer::addIndices(uint16_t* data, const size_t size)
{
	ASSERT(!isCommitted_);
	const size_t prevSize = indices_.size();
	indices_.resize(prevSize + size);
	std::copy_n(data, size, indices_.begin() + prevSize);
	return prevSize;
}

VkBuffer ElementBuffer::getVertexBuffer()
{
	if (isCommitted_)
		return vertexBufferDetails_.buffer;
	else
		return VK_NULL_HANDLE;
}

VkBuffer ElementBuffer::getIndexBuffer()
{
	if (isCommitted_)
		return indexBufferDetails_.buffer;
	else
		return VK_NULL_HANDLE;
}

uint32_t ElementBuffer::indexCount()
{
	return indexCount_;
}

void ElementBuffer::bind(VkCommandBuffer cmdBuffer)
{
	VkBuffer vertexBuffers[] = { getVertexBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(cmdBuffer, getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
}

void ElementBuffer::emplaceMesh(std::string name, size_t indexCount, size_t indexOffset, size_t vertexOffset)
{
	meshes_[name] = new BasicMesh();
	meshes_[name]->indexCount = indexCount;
	meshes_[name]->indexOffset = indexOffset;
	meshes_[name]->vertexOffset = vertexOffset;
}

const BasicMesh* ElementBuffer::operator[](const std::string& name) const
{
	ASSERT(meshes_.count(name) > 0);
	return meshes_.at(name);
}
