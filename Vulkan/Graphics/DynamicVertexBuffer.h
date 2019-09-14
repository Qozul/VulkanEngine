#pragma once
#include "VkUtil.h"
#include "Mesh.h"
#include "DeviceMemory.h"
#include "ElementBufferInterface.h"

namespace QZL {
	namespace Graphics {
		// This class provides a vertex buffer which gives out ranges for animated mesh systems to use
		// and update each frame.
		template<typename V>
		class DynamicVertexBuffer : public VertexBufferInterface, public DynamicBufferInterface {
		public:
			DynamicVertexBuffer(DeviceMemory* deviceMemory, const size_t maxVertices, const int numSwapChainImages)
				: deviceMemory_(deviceMemory), maxVertices_(maxVertices), usedVertices_(0) {
				vertexBufferDetails_ = deviceMemory_->createBuffer(MemoryAllocationPattern::kDynamicResource, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, maxVertices * sizeof(V) * numSwapChainImages);
				ASSERT(vertexBufferDetails_.mappedData != nullptr);
			}
			~DynamicVertexBuffer() {
				deviceMemory_->deleteAllocation(vertexBufferDetails_.id, vertexBufferDetails_.buffer);
				for (auto mesh : meshes_) {
					SAFE_DELETE(mesh.second);
				}
			}

			void updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx) override;
			void bind(VkCommandBuffer cmdBuffer) override;
			void addVertices(void* data, const size_t count, const size_t offset = std::numeric_limits<size_t>::max()) override;
			void emplaceMesh(std::string name, size_t indexCount, size_t indexOffset, size_t vertexOffset) override;
			bool contains(const std::string& id) override {
				return meshes_.count(name) > 0;
			}

			SubBufferRange allocateSubBufferRange(size_t vertexCount) override;
			void freeSubBufferRange(VertexSubBufferRange range) {} // TODO

			void* getSubBufferData(size_t firstVertex) override {
				return &buffer_[firstVertex];
			}

			const BasicMesh* operator[](const std::string& name) const;
		private:
			MemoryAllocationDetails vertexBufferDetails_;
			DeviceMemory* deviceMemory_;
			const size_t maxVertices_;
			size_t usedVertices_;

			// Updates from any vertex system is pushed in to this buffer, then each frame it is copied to the appropriate section of the vertex buffer.
			// It always contains the most recent vertex values
			std::vector<V> buffer_;
		};

		template<typename V>
		inline void DynamicVertexBuffer<V>::updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx)
		{
			// Update the vertex range for this frame
			memcpy(vertexBufferDetails_.mappedData[maxVertices * idx], buffer_.data(), maxVertices * sizeof(V));
		}

		template<typename V>
		inline void DynamicVertexBuffer<V>::bind(VkCommandBuffer cmdBuffer)
		{
			VkDeviceSize offsets[] = { maxVertices * sizeof(V) * idx };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBufferDetails_.buffer, offsets);
		}

		template<typename V>
		inline void DynamicVertexBuffer<V>::addVertices(void* data, const size_t count, const size_t offset)
		{
			size_t prevSize;
			if (offset == std::numeric_limits<size_t>::max()) {
				prevSize = offset;
			}
			else {
				prevSize = vertices.size();
			}
			vertices_.resize(prevSize + size);
			std::copy_n(static_cast<V*>(data), size, vertices_.begin() + prevSize);
			return prevSize;
		}

		template<typename V>
		inline void DynamicVertexBuffer<V>::emplaceMesh(std::string name, size_t count, size_t indexOffset, size_t vertexOffset)
		{
			meshes_[name] = new BasicMesh();
			meshes_[name]->count = indexCount;
			meshes_[name]->indexOffset = indexOffset;
			meshes_[name]->vertexOffset = vertexOffset;
		}

		template<typename V>
		inline SubBufferRange DynamicVertexBuffer<V>::allocateSubBufferRange(size_t vertexCount)
		{
			ASSERT(usedVertices_ + vertexCount <= maxVertices_);

			VertexSubBufferRange range = {};
			range.firstVertex = usedVertices_;
			range.vertexCount = vertexCount;

			usedVertices_ += vertexCount;
			return range;
		}
		template<typename V>
		inline const BasicMesh* DynamicVertexBuffer<V>::operator[](const std::string& name) const
		{
			ASSERT(meshes_.count(name) > 0);
			return meshes_.at(name);
		}
	}
}
