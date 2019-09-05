#pragma once
#include "ElementBufferInterface.h"
#include "Mesh.h"
#include "DeviceMemory.h"

namespace QZL
{
	namespace Graphics {
		// Element buffer is immutable, once committed it cannot be changed
		template<typename V>
		class ElementBuffer : public ElementBufferInterface {
		public:
			ElementBuffer(DeviceMemory* deviceMemory);
			~ElementBuffer();
			void commit() override;
			const bool isCommitted() override;

			size_t addVertices(void* data, const size_t size) override;
			size_t addIndices(uint16_t* data, const size_t size) override;
			VkBuffer getVertexBuffer() override;
			VkBuffer getIndexBuffer() override;
			uint32_t indexCount() override;
			void bind(VkCommandBuffer cmdBuffer) override;
			BasicMesh* getMesh(std::string name) override {
				return meshes_[name];
			}

			void emplaceMesh(std::string name, size_t indexCount, size_t indexOffset, size_t vertexOffset) override;
			const BasicMesh* operator[](const std::string& name) const;
			bool contains(const std::string& name) override {
				return meshes_.count(name) > 0;
			}
		private:
			MemoryAllocationDetails vertexBufferDetails_;
			MemoryAllocationDetails indexBufferDetails_;
			DeviceMemory* deviceMemory_;

			uint32_t indexCount_;
			std::vector<uint16_t> indices_;
			std::vector<V> vertices_;
			std::map<std::string, BasicMesh*> meshes_;
			bool isCommitted_;
		};

		template<typename V>
		inline ElementBuffer<V>::ElementBuffer(DeviceMemory* deviceMemory)
			: deviceMemory_(deviceMemory), isCommitted_(false), indexCount_(0)
		{
		}

		template<typename V>
		inline ElementBuffer<V>::~ElementBuffer()
		{
			if (isCommitted_) {
				deviceMemory_->deleteAllocation(vertexBufferDetails_.id, vertexBufferDetails_.buffer);
				deviceMemory_->deleteAllocation(indexBufferDetails_.id, indexBufferDetails_.buffer);
			}
		}

		template<typename V>
		inline void ElementBuffer<V>::commit()
		{
			if (isCommitted_) {
				return;
			}

			size_t size = indices_.size() * sizeof(uint16_t);
			size_t size2 = vertices_.size() * sizeof(V);
			size_t largestSize = size > size2 ? size : size2;
			if (largestSize == 0) {
				return;
			}
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

		template<typename V>
		inline const bool ElementBuffer<V>::isCommitted()
		{
			return isCommitted_;
		}

		template<typename V>
		inline size_t ElementBuffer<V>::addVertices(void* data, const size_t size)
		{
			ASSERT(!isCommitted_);
			const size_t prevSize = vertices_.size();
			vertices_.resize(prevSize + size);
			std::copy_n(static_cast<V*>(data), size, vertices_.begin() + prevSize);
			return prevSize;
		}

		template<typename V>
		inline size_t ElementBuffer<V>::addIndices(uint16_t* data, const size_t size)
		{
			ASSERT(!isCommitted_);
			const size_t prevSize = indices_.size();
			indices_.resize(prevSize + size);
			std::copy_n(data, size, indices_.begin() + prevSize);
			return prevSize;
		}

		template<typename V>
		inline VkBuffer ElementBuffer<V>::getVertexBuffer()
		{
			if (isCommitted_)
				return vertexBufferDetails_.buffer;
			else
				return VK_NULL_HANDLE;
		}

		template<typename V>
		inline VkBuffer ElementBuffer<V>::getIndexBuffer()
		{
			if (isCommitted_)
				return indexBufferDetails_.buffer;
			else
				return VK_NULL_HANDLE;
		}

		template<typename V>
		inline uint32_t ElementBuffer<V>::indexCount()
		{
			return indexCount_;
		}

		template<typename V>
		inline void ElementBuffer<V>::bind(VkCommandBuffer cmdBuffer)
		{
			VkBuffer vertexBuffers[] = { getVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
		}

		template<typename V>
		inline void ElementBuffer<V>::emplaceMesh(std::string name, size_t indexCount, size_t indexOffset, size_t vertexOffset)
		{
			meshes_[name] = new BasicMesh();
			meshes_[name]->indexCount = indexCount;
			meshes_[name]->indexOffset = indexOffset;
			meshes_[name]->vertexOffset = vertexOffset;
		}

		template<typename V>
		inline const BasicMesh* ElementBuffer<V>::operator[](const std::string& name) const
		{
			ASSERT(meshes_.count(name) > 0);
			return meshes_.at(name);
		}
	}
}
