// Author: Ralph Ridley
// Date: 19/02/19
// Purpose: Encapsulate and support loading meshes from .obj files.
#pragma once
#include "Vertex.h"

namespace QZL
{
	namespace Graphics {
		using IndexType = uint16_t;

		class ElementBufferObject;
		struct BasicMesh;

		class MeshLoader {
		public:
			static BasicMesh* loadMesh(const std::string& meshName, ElementBufferObject& eleBuf, MeshLoadFunc loaderFunc);
		private:
			static void placeMeshInBuffer(const std::string& meshName, ElementBufferObject& eleBuf, uint32_t count, 
				void* indices, void* vertices, size_t indicesSize, size_t verticesSize);
			static void loadMeshFromFile(const std::string& meshName, ElementBufferObject& eleBuf);
			static const std::string kPath;
			static const std::string kExt;
		};
	}
}