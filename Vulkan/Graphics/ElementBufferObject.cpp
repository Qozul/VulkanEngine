// Author: Ralph Ridley
// Date: 03/11/19
#include "ElementBufferObject.h"
#include "DeviceMemory.h"
#include "Mesh.h"

using namespace QZL;
using namespace QZL::Graphics;

ElementBufferObject::ElementBufferObject(DeviceMemory* deviceMemory, size_t sizeOfVertices, size_t sizeOfIndices)
	: deviceMemory_(deviceMemory), sizeOfVertices_(sizeOfVertices), sizeOfIndices_(sizeOfIndices), isDynamic_(false), isCommitted_(false),
	  indexCount_(0), vertexCount_(0)
{
	ASSERT(sizeOfVertices != 0 && sizeOfIndices <= 4 && sizeOfIndices != 3);

	indexType_ = sizeOfIndices == 2 ? VK_INDEX_TYPE_UINT16 : 
		sizeOfIndices == 4 ? VK_INDEX_TYPE_UINT32 :
		sizeOfIndices == 1 ? VK_INDEX_TYPE_UINT8_EXT : 
		VK_INDEX_TYPE_NONE_NV;

	vertexBufferDetails_.buffer = VK_NULL_HANDLE;
	indexBufferDetails_.buffer = VK_NULL_HANDLE;
}

ElementBufferObject::ElementBufferObject(DeviceMemory* deviceMemory)
	: deviceMemory_(deviceMemory), sizeOfVertices_(0), sizeOfIndices_(0), isDynamic_(false), isCommitted_(true), indexCount_(0), vertexCount_(0), indexType_(VK_INDEX_TYPE_NONE_NV)
{
	vertexBufferDetails_ = deviceMemory_->createBuffer("EBO VertexBuffer", MemoryAllocationPattern::kStaticResource, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 1);
}

ElementBufferObject::~ElementBufferObject()
{
	if (isCommitted_) {
		deviceMemory_->deleteAllocation(vertexBufferDetails_.id, vertexBufferDetails_.buffer);
		if (isIndexed()) {
			deviceMemory_->deleteAllocation(indexBufferDetails_.id, indexBufferDetails_.buffer);
		}
	}
	for (auto mesh : meshes_) {
		SAFE_DELETE(mesh.second);
	}
}

void ElementBufferObject::bind(VkCommandBuffer cmdBuffer, const size_t idx)
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBufferDetails_.buffer, &offset);
	if (isIndexed()) {
		vkCmdBindIndexBuffer(cmdBuffer, getIndexBuffer(), 0, indexType_);
	}
}

void ElementBufferObject::commit()
{
	if (isCommitted_) {
		return;
	}

	size_t indexSize = indexData_.size();
	size_t vertexSize = vertexData_.size();
	size_t largestSize = indexSize > vertexSize ? indexSize : vertexSize;
	if (largestSize == 0) {
		return;
	}

	MemoryAllocationDetails stagingBuffer = deviceMemory_->createBuffer("", MemoryAllocationPattern::kStaging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, largestSize);

	void* data = deviceMemory_->mapMemory(stagingBuffer.id);
	memcpy(data, vertexData_.data(), vertexSize);
	deviceMemory_->unmapMemory(stagingBuffer.id);
	vertexBufferDetails_ = deviceMemory_->createBuffer("EBO VertexBuffer", MemoryAllocationPattern::kStaticResource, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexSize);
	deviceMemory_->transferMemory(stagingBuffer.buffer, vertexBufferDetails_.buffer, 0, 0, vertexSize);
	if (isIndexed()) {
		data = deviceMemory_->mapMemory(stagingBuffer.id);
		memcpy(data, indexData_.data(), indexSize);
		deviceMemory_->unmapMemory(stagingBuffer.id);
		indexBufferDetails_ = deviceMemory_->createBuffer("EBO IndexBuffer", MemoryAllocationPattern::kStaticResource, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexSize);
		deviceMemory_->transferMemory(stagingBuffer.buffer, indexBufferDetails_.buffer, 0, 0, indexSize);
		indexData_.clear();
	}

	deviceMemory_->deleteAllocation(stagingBuffer.id, stagingBuffer.buffer);

	if (!isDynamic()) {
		vertexData_.clear();
	}
	isCommitted_ = true;
}

size_t ElementBufferObject::addVertices(void* data, const size_t size)
{
	ASSERT(!isCommitted_ && ((size % sizeOfVertices_) == 0));
	const size_t prevSize = vertexData_.size();
	vertexData_.resize(prevSize + size);
	if (data != nullptr) {
		memcpy(vertexData_.data() + prevSize, data, size);
	}
	vertexCount_ += static_cast<uint32_t>(size / sizeOfVertices_);
	ASSERT(vertexCount_ == (vertexData_.size() / sizeOfVertices_));
	return prevSize;
}

size_t ElementBufferObject::addIndices(void* data, const size_t size)
{
	ASSERT(!isCommitted_ && ((size % sizeOfIndices_) == 0));
	const size_t prevSize = indexData_.size();
	indexData_.resize(prevSize + size);
	if (data != nullptr) {
		memcpy(indexData_.data() + prevSize, data, size);
	}
	indexCount_ += static_cast<uint32_t>(size / sizeOfIndices_);
	ASSERT(indexCount_ == (indexData_.size() / sizeOfIndices_));
	return prevSize;
}

void ElementBufferObject::emplaceMesh(const std::string name, uint32_t count, uint32_t vertexOffset, uint32_t indexOffset)
{
	meshes_[name] = new BasicMesh();
	meshes_[name]->count = count;
	meshes_[name]->indexOffset = indexOffset;
	meshes_[name]->vertexOffset = vertexOffset;
}
