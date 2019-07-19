#pragma once
#include "VkUtil.h"
#include "MemoryAllocation.h"
#include "Mesh.h"

namespace QZL
{
	namespace Graphics {
		class DeviceMemory;

		// Element buffer is immutable, once committed it cannot be changed
		class ElementBuffer {
			friend class MeshLoader;
		public:
			ElementBuffer(DeviceMemory* deviceMemory);
			~ElementBuffer();
			void commit();
			const bool isCommitted();

			size_t addVertices(Vertex* data, const size_t size);
			size_t addIndices(uint16_t* data, const size_t size);
			VkBuffer getVertexBuffer();
			VkBuffer getIndexBuffer();
			uint32_t indexCount();
			void bind(VkCommandBuffer cmdBuffer);

			void emplaceMesh(std::string name, size_t indexCount, size_t indexOffset, size_t vertexOffset);
			template<typename InstType>
			InstType* getNewMeshInstance(const std::string& name);
			const BasicMesh* operator[](const std::string& name) const;
			bool contains(const std::string& name) {
				return meshes_.count(name) > 0;
			}
		private:
			MemoryAllocationDetails vertexBufferDetails_;
			MemoryAllocationDetails indexBufferDetails_;
			DeviceMemory* deviceMemory_;

			uint32_t indexCount_;
			std::vector<uint16_t> indices_;
			std::vector<Vertex> vertices_;
			std::map<std::string, BasicMesh*> meshes_;
			bool isCommitted_;
		};
		template<typename InstType>
		inline InstType* ElementBuffer::getNewMeshInstance(const std::string& name)
		{
			ASSERT(meshes_.count(name) > 0);
			return makeMeshInstance<InstType>(name, *meshes_[name]);
		}
	}
}
