#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Graphics {
		class BasicMesh;
		class ElementBufferInterface {
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
		};
	}
}
