#pragma once
#include "Vertex.h"
#include "../Assets/Transform.h"

namespace QZL
{
	namespace Graphics {
		class Texture;

		/// BasicMesh needs to provide a transform and pointers to it's data
		struct BasicMesh {
			uint32_t count; // This will be index count for indexed data, vertex count otherwise
			uint32_t indexOffset; // Index offset is only used for indexed data
			uint32_t vertexOffset;
		};
		/*
		struct MeshInstance {
			Transform transform;
		};
		struct TexturedMeshInstance {
			Texture* texture;
			Transform transform;
		};
		template<typename InstType>
		inline InstType* makeMeshInstance(const std::string& meshName, const BasicMesh& mesh)
		{
			static_assert(std::is_same<InstType, MeshInstance>::value ||
				std::is_same<InstType, TexturedMeshInstance>::value,
				"Type not allowed");

			InstType* inst = new InstType();
			return inst;
		}*/
	}
}