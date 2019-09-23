#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Graphics {
		struct BasicMesh;

		struct SubBufferRange {
			size_t first;
			size_t count;
		};

		enum class BufferFlags : int {
			ELEMENT = 1,
			VERTEX = 2, 
			DYNAMIC = 4
		};

		// Bit operators to correctly treat the enum as flags
		inline BufferFlags operator|(BufferFlags a, BufferFlags b)
		{
			return static_cast<BufferFlags>(static_cast<int>(a) | static_cast<int>(b));
		}
		inline bool operator&(BufferFlags a, BufferFlags b)
		{
			return static_cast<int>(a) & static_cast<int>(b);
		}

		class BufferInterface {
		public:
			BufferInterface() : flags_(static_cast<BufferFlags>(0)) {}
			BufferFlags bufferType() {
				return flags_;
			}
			virtual ~BufferInterface() {}
			virtual void bind(VkCommandBuffer cmdBuffer, const size_t idx) = 0;
			virtual void commit() = 0;
		protected:
			BufferFlags flags_;
		};

		class ElementBufferInterface : public BufferInterface {
		public:
			ElementBufferInterface() {
				flags_ = flags_ | BufferFlags::ELEMENT;
				ASSERT(!(flags_ & BufferFlags::VERTEX));
			}

			virtual size_t addVertices(void* data, const size_t size) = 0;
			virtual size_t addIndices(uint16_t* data, const size_t size) = 0;
			virtual VkBuffer getVertexBuffer() = 0;
			virtual VkBuffer getIndexBuffer() = 0;
			virtual void emplaceMesh(std::string name, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) = 0;
			virtual bool contains(const std::string& name) = 0;
			virtual BasicMesh* getMesh(std::string name) = 0;
			virtual uint32_t indexCount() = 0;
			virtual const bool isCommitted() = 0;
			virtual ~ElementBufferInterface() { }
		};

		// Virtual base since dynamic should be an addition to a concrete buffer type
		class DynamicBufferInterface : public virtual BufferInterface {
		public:
			DynamicBufferInterface() {
				flags_ = flags_ | BufferFlags::DYNAMIC;
			}
			virtual ~DynamicBufferInterface() {}

			virtual void updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx) = 0;
			virtual SubBufferRange allocateSubBufferRange(size_t count) = 0;
			virtual void* getSubBufferData(size_t firstVertex) = 0;
		};

		class VertexBufferInterface : public virtual BufferInterface {
		public:
			VertexBufferInterface() {
				flags_ = flags_ | BufferFlags::VERTEX;
				ASSERT(!(flags_ & BufferFlags::ELEMENT));
			}
			virtual ~VertexBufferInterface() {}

			virtual void addVertices(void* data, const size_t count) = 0;
			virtual VkBuffer getVertexBuffer() = 0;
			virtual void emplaceMesh(std::string name, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) = 0;
			virtual bool contains(const std::string& id) = 0;
			virtual uint32_t count() = 0;
		};
	}
}
