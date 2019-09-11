#pragma once
#include "VkUtil.h"
#include "Mesh.h"
#include "DeviceMemory.h"

namespace QZL {
	namespace Graphics {

		struct VertexSubBufferRange {
			size_t firstVertex;
			size_t vertexCount;
		};

		// This class provides a vertex buffer which gives out ranges for animated mesh systems to use
		// and update each frame.
		template<typename V>
		class DynamicVertexBuffer {
		public:
			DynamicVertexBuffer(DeviceMemory* deviceMemory, const size_t maxVertices, const int numSwapChainImages)
				: deviceMemory_(deviceMemory), maxVertices_(maxVertices), usedVertices_(0) {
				vertexBufferDetails_ = deviceMemory_->createBuffer(MemoryAllocationPattern::kDynamicResource, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, maxVertices * sizeof(V) * numSwapChainImages);
				ASSERT(vertexBufferDetails_.mappedData != nullptr);
			}
			~DynamicVertexBuffer() {
				deviceMemory_->deleteAllocation(vertexBufferDetails_.id, vertexBufferDetails_.buffer);
			}

			void updateAndBind(VkCommandBuffer cmdBuffer, const uint32_t idx);

			VertexSubBufferRange allocateSubBufferRange(size_t vertexCount);
			void freeSubBufferRange(VertexSubBufferRange range) {} // TODO

			V* getSubBuffer(size_t firstVertex) {
				return &buffer_[firstvertex];
			}
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
		inline void DynamicVertexBuffer<V>::updateAndBind(VkCommandBuffer cmdBuffer, const uint32_t idx)
		{
			// Update the vertex range for this frame
			memcpy(vertexBufferDetails_.mappedData[maxVertices * idx], buffer_.data(), maxVertices * sizeof(V));

			// Bind the vertex buffer range for this frame
			VkDeviceSize offsets[] = { maxVertices * sizeof(V) * idx };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBufferDetails_.buffer, offsets);
		}

		template<typename V>
		inline VertexSubBufferRange DynamicVertexBuffer<V>::allocateSubBufferRange(size_t vertexCount)
		{
			ASSERT(usedVertices_ + vertexCount <= maxVertices_);

			VertexSubBufferRange range = {};
			range.firstVertex = usedVertices_;
			range.vertexCount = vertexCount;

			usedVertices_ += vertexCount;
			return range;
		}
	}
}
