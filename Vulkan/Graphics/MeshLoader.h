/// Author: Ralph Ridley
/// Date: 19/02/19
/// Purpose: Encapsulate and support loading meshes from .obj files.
#pragma once
#include "ElementBuffer.h"

namespace QZL
{
	namespace Graphics {
		using IndexType = uint16_t;
		using MeshLoaderFunction = void(*)(std::vector<IndexType>& indices, std::vector<Vertex>& vertices);

		class MeshLoader {
		public:
			static BasicMesh* loadMesh(const std::string& meshName, ElementBuffer& eleBuf, MeshLoaderFunction loadFunc = nullptr);
		private:
			static void placeMeshInBuffer(const std::string& meshName, ElementBuffer& eleBuf, std::vector<IndexType>& indices, std::vector<Vertex>& vertices);
			static void loadMeshFromFile(const std::string& meshName, ElementBuffer& eleBuf);
			static const std::string kPath;
			static const std::string kExt;
		};
	}
}