#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Graphics {
		class BasicMesh;

		struct SubBufferRange {
			size_t first;
			size_t count;
		};

		enum class BufferType {
			ELEMENT, VERTEX
		};

		class BufferInterface {
		public:
			virtual BufferType bufferType() = 0;
		};

		class ElementBufferInterface : public BufferInterface {
		public:
			virtual void bind(VkCommandBuffer cmdBuffer) = 0;
			virtual size_t addVertices(void* data, const size_t size) = 0;
			virtual size_t addIndices(uint16_t* data, const size_t size) = 0;
			virtual VkBuffer getVertexBuffer() = 0;
			virtual VkBuffer getIndexBuffer() = 0;
			virtual void emplaceMesh(std::string name, size_t indexCount, size_t indexOffset, size_t vertexOffset) = 0;
			virtual bool contains(const std::string& name) = 0;
			virtual BasicMesh* getMesh(std::string name) = 0;
			virtual uint32_t indexCount() = 0;
			virtual const bool isCommitted() = 0;
			virtual void commit() = 0;
			virtual ~ElementBufferInterface() { }

			BufferType bufferType() {
				return BufferType::ELEMENT;
			}
		};

		class DynamicBufferInterface {
		public:
			virtual void updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx) = 0;
			virtual SubBufferRange allocateSubBufferRange(size_t count) = 0;
			virtual void* getSubBufferData(size_t firstVertex) = 0;
		protected:
			virtual ~DynamicBufferInterface() {}
		};

		class VertexBufferInterface {
		public:
			virtual void bind(VkCommandBuffer cmdBuffer) = 0;
			// Offset can be negative, with < 0 representing no offset given
			virtual void addVertices(void* data, const size_t count, const size_t offset) = 0;
			virtual VkBuffer getVertexBuffer() = 0;
			virtual void emplaceMesh(std::string name, size_t indexCount, size_t indexOffset, size_t vertexOffset) = 0;
			virtual bool contains(const std::string& id) = 0;
			virtual uint32_t count() = 0;
			virtual ~VertexBufferInterface() {}

			BufferType bufferType() {
				return BufferType::VERTEX;
			}
		};
	}
}
