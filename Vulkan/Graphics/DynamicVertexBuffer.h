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
				
			}
			~DynamicVertexBuffer() {
				deviceMemory_->deleteAllocation(vertexBufferDetails_.id, vertexBufferDetails_.buffer);
				for (auto mesh : meshes_) {
					SAFE_DELETE(mesh.second);
				}
			}

			void updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx) override;
			void bind(VkCommandBuffer cmdBuffer, const size_t idx) override;
			void addVertices(void* data, const size_t count) override;
			void emplaceMesh(std::string name, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) override;
			bool contains(const std::string& id) override {
				return meshes_.count(id) > 0;
			}

			SubBufferRange allocateSubBufferRange(size_t vertexCount) override;
			void freeSubBufferRange(SubBufferRange range) {} // TODO

			void* getSubBufferData(size_t firstVertex) override {
				return &buffer_[firstVertex];
			}

			void commit() override {}

			uint32_t count() override;

			VkBuffer getVertexBuffer() {
				return vertexBufferDetails_.buffer;
			}

			const BasicMesh* operator[](const std::string& name) const;
		private:
			MemoryAllocationDetails vertexBufferDetails_;
			DeviceMemory* deviceMemory_;
			const size_t maxVertices_;
			size_t usedVertices_;
			std::map<std::string, BasicMesh*> meshes_;

			// Updates from any vertex system is pushed in to this buffer, then each frame it is copied to the appropriate section of the vertex buffer.
			// It always contains the most recent vertex values
			std::vector<V> buffer_;
		};

		template<typename V>
		inline void DynamicVertexBuffer<V>::updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx)
		{
			// Update the vertex range for this frame
			if (vertexBufferDetails_.mappedData != nullptr) {
				memcpy(static_cast<V*>(vertexBufferDetails_.mappedData) + (idx * maxVertices_), static_cast<V*>(buffer_.data()), maxVertices_ * sizeof(V));
			}
			else {
				V* dataPtr = static_cast<V*>(deviceMemory_->mapMemory(vertexBufferDetails_.id));
				memcpy(static_cast<V*>(dataPtr) + (idx * maxVertices_), buffer_.data(), maxVertices_ * sizeof(V));
				deviceMemory_->unmapMemory(vertexBufferDetails_.id);
			}
		}

		template<typename V>
		inline void DynamicVertexBuffer<V>::bind(VkCommandBuffer cmdBuffer, const size_t idx)
		{
			auto n = sizeof(V);
			VkDeviceSize offsets[] = { maxVertices_ * sizeof(V) * idx };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBufferDetails_.buffer, offsets);
		}

		template<typename V>
		inline void DynamicVertexBuffer<V>::addVertices(void* data, const size_t count)
		{
			size_t prevSize = buffer_.size();
			buffer_.resize(prevSize + count);
			if (data != nullptr) {
				std::copy_n(static_cast<V*>(data), count, buffer_.begin() + prevSize);
			}
		}

		template<typename V>
		inline void DynamicVertexBuffer<V>::emplaceMesh(std::string name, uint32_t count, uint32_t indexOffset, uint32_t vertexOffset)
		{
			meshes_[name] = new BasicMesh();
			meshes_[name]->count = count;
			meshes_[name]->indexOffset = indexOffset;
			meshes_[name]->vertexOffset = vertexOffset;
		}

		template<typename V>
		inline SubBufferRange DynamicVertexBuffer<V>::allocateSubBufferRange(size_t vertexCount)
		{
			ASSERT(usedVertices_ + vertexCount <= maxVertices_);

			SubBufferRange range = {};
			range.first = usedVertices_;
			range.count = vertexCount;

			usedVertices_ += vertexCount;
			addVertices(nullptr, vertexCount);
			return range;
		}
		template<typename V>
		inline uint32_t DynamicVertexBuffer<V>::count()
		{
			return static_cast<uint32_t>(usedVertices_);
		}
		template<typename V>
		inline const BasicMesh* DynamicVertexBuffer<V>::operator[](const std::string& name) const
		{
			ASSERT(meshes_.count(name) > 0);
			return meshes_.at(name);
		}
	}
}
