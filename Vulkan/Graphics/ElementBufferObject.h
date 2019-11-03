// Author: Ralph Ridley
// Date: 03/11/19
#pragma once
#include "VkUtil.h"
#include "MemoryAllocation.h"

namespace QZL {
	namespace Graphics {
		class DeviceMemory;
		struct BasicMesh;

		struct SubBufferRange {
			size_t first;
			size_t count;
		};

		/*
			An element buffer defines the vertex data for a group of meshes. It may optionally include indices for when the mesh uses indexing, and it may
			be dynamic (updatable each frame) if using the DynamicElementBuffer subclass. The class does not ensure given data is of the correct size or type,
			only confirming that data passed in is a multiple of sizeOf parameters passed in on construction. Passing in incorrect data may lead to undefined behaviour.

			Before use by the GPU, commit() must be called. An error will be thrown if bind() is called while the buffer is not committed. Upon commit(), the
			size of the buffer is locked in, GPU side buffers are created, and CPU side data is cleared (unless the buffer is dynamic). If the buffer is static then 
			it cannot be updated after commit().
		*/
		class ElementBufferObject {
			friend class MeshLoader;
		public:
			ElementBufferObject(DeviceMemory* deviceMemory, size_t sizeOfVertices, size_t sizeOfIndices = 0);
			virtual ~ElementBufferObject();

			virtual void bind(VkCommandBuffer cmdBuffer, const size_t idx);
			virtual void commit();

			// Parameter "size" should be the sizeof the data type * the number of data elements
			size_t addVertices(void* data, const size_t size);
			size_t addIndices(void* data, const size_t size);

			// When using vertex only, count is the number of vertices. When using indexed data, count is the number of indices.
			void emplaceMesh(const std::string name, uint32_t count, uint32_t vertexOffset, uint32_t indexOffset = 0);

			bool isIndexed() {
				return sizeOfIndices_ > 0;
			}

			bool isDynamic() {
				return isDynamic_;
			}

			const bool isCommitted() {
				return isCommitted_;
			}

			VkBuffer getVertexBuffer() {
				return vertexBufferDetails_.buffer;
			}

			VkBuffer getIndexBuffer() {
				return indexBufferDetails_.buffer;
			}

			uint32_t indexCount() {
				return indexCount_;
			}

			uint32_t vertexCount() {
				return vertexCount_;
			}

			BasicMesh* getMesh(const std::string name)
			{
				return meshes_[name];
			}

			bool containsMesh(const std::string& name)
			{
				return meshes_.count(name) > 0;
			}

			// These are defined here to avoid dynamic casts at runtime for dynamic buffers
			virtual void updateBuffer(VkCommandBuffer& cmdBuffer, const uint32_t& idx) { }
			virtual SubBufferRange allocateSubBufferRange(size_t count) { return { 0, 0 }; }
			virtual void* getSubBufferData(size_t firstVertex) { return nullptr; }

		protected:
			const size_t sizeOfVertices_;
			const size_t sizeOfIndices_;
			bool isDynamic_;
			bool isCommitted_;

			MemoryAllocationDetails vertexBufferDetails_;
			MemoryAllocationDetails indexBufferDetails_;
			DeviceMemory* deviceMemory_;
			VkIndexType indexType_;

			uint32_t vertexCount_;
			uint32_t indexCount_;
			std::vector<char> indexData_;
			std::vector<char> vertexData_;
			std::map<std::string, BasicMesh*> meshes_;
		};
	}
}
