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
			RenderObject(ElementBufferObject* meshBuffer, const std::string meshName, ShaderParams* params, MeshLoadFunc loadFunc, Material* material)
				: meshName_(meshName), mesh_(MeshLoader::loadMesh(meshName, *meshBuffer, loadFunc)), params_(params), material_(material), deleteMesh_(false) {
				key_ = material != nullptr ? meshName : meshName;
				key_ = params != nullptr ? key_ + params->id : key_;
			}
			RenderObject(const std::string name, BasicMesh* mesh, ShaderParams* params, Material* material, bool deleteMesh = true)
				: meshName_(name), mesh_(mesh), params_(params), material_(material), deleteMesh_(deleteMesh) {
				key_ = material != nullptr ? name : name;
				key_ = params != nullptr ? key_ + params->id : key_;
			}
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