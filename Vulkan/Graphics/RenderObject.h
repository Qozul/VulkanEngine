// Author: Ralph Ridley
// Date: 12/10/19
// Define a render object to provide a neat way of encapsulating per draw call parameters and a mesh

#pragma once
#include "Mesh.h"
#include "MeshLoader.h"
#include "ShaderParams.h"

namespace QZL {
	namespace Graphics {
		class RenderObject {
		public:
			RenderObject(ElementBufferInterface* meshBuffer, const std::string meshName, ShaderParams* params, MeshLoadingInfo mlInfo, Material* material)
				: meshName_(meshName), mesh_(MeshLoader::loadMesh(meshName, *meshBuffer, mlInfo)), params_(params), key_(meshName + params->id + material->getName()), 
				material_(material), deleteMesh_(false) { }
			RenderObject(const std::string name, BasicMesh* mesh, ShaderParams* params, Material* material, bool deleteMesh = true)
				: meshName_(name), mesh_(mesh), params_(params), key_(name + params->id + material->getName()), material_(material), deleteMesh_(deleteMesh) { }
			~RenderObject() {
				if (deleteMesh_) {
					SAFE_DELETE(mesh_);
				}
				SAFE_DELETE(params_);
			}

			BasicMesh* getMesh() {
				return mesh_;
			}

			ShaderParams* getParams() {
				return params_;
			}

			std::string& getKey() {
				return key_;
			}

			std::string& getMeshName() {
				return meshName_;
			}

			Material* getMaterial() {
				return material_;
			}

		private:
			std::string key_;
			std::string meshName_;
			BasicMesh* mesh_;
			ShaderParams* params_;
			Material* material_;
			const bool deleteMesh_;
		};
	}
}