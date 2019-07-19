/// Author: Ralph Ridley
/// Date: 19/02/19
/// Purpose: Encapsulate and support loading meshes from .obj files.
#pragma once
#include "ElementBuffer.h"

namespace QZL
{
	namespace Graphics {
		class MeshLoader {
		public:
			static BasicMesh* loadMesh(const std::string& meshName, ElementBuffer& eleBuf);
		private:
			static void loadMeshFromFile(const std::string& meshName, ElementBuffer& eleBuf);
			static const std::string kPath;
			static const std::string kExt;
		};
	}
}