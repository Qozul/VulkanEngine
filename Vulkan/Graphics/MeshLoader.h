/// Author: Ralph Ridley
/// Date: 19/02/19
/// Purpose: Encapsulate and support loading meshes from .obj files.
#pragma once
#include "ElementBuffer.h"

namespace QZL
{
	namespace Graphics {
		using IndexType = uint16_t;
		using MeshLoadFuncFull = void(*)(std::vector<IndexType>& indices, std::vector<Vertex>& vertices);
		using MeshLoadFuncOnlyPos = void(*)(std::vector<IndexType>& indices, std::vector<VertexOnlyPosition>& vertices);

		enum class MeshLoadFuncType {
			NONE, FULL, ONLY_POS
		};

		struct MeshLoadingInfo {
			MeshLoadFuncType type;
			union {
				MeshLoadFuncFull full;
				MeshLoadFuncOnlyPos onlyPos;
			} meshLoaderFunction;

			MeshLoadingInfo()
				: type(MeshLoadFuncType::NONE) {
				meshLoaderFunction.full = nullptr;
			}

			MeshLoadingInfo(MeshLoadFuncFull func) 
				: type(MeshLoadFuncType::FULL) {
				meshLoaderFunction.full = func;
			}
			MeshLoadingInfo(MeshLoadFuncOnlyPos func)
				: type(MeshLoadFuncType::ONLY_POS) {
				meshLoaderFunction.onlyPos = func;
			}
		};

		class MeshLoader {
		public:
			static BasicMesh* loadMesh(const std::string& meshName, ElementBufferInterface& eleBuf, MeshLoadingInfo& mlInfo);
		private:
			template<typename V>
			static void placeMeshInBuffer(const std::string& meshName, ElementBufferInterface& eleBuf, std::vector<IndexType>& indices, std::vector<V>& vertices) {
				auto indexOffset = eleBuf.addIndices(indices.data(), indices.size());
				auto vertexOffset = eleBuf.addVertices(vertices.data(), vertices.size());
				eleBuf.emplaceMesh(meshName, indices.size(), indexOffset, vertexOffset);
			}
			static void loadMeshFromFile(const std::string& meshName, ElementBufferInterface& eleBuf);
			static const std::string kPath;
			static const std::string kExt;
		};
	}
}