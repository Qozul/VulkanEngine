// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

namespace QZL {
	class Entity;
	namespace Graphics {
		struct ShaderParams;
		struct Material;
		struct BasicMesh;
		class RenderObject;
		class GraphicsComponent {
			friend class Entity;
		public:
			GraphicsComponent(Entity* owner, RendererTypes type, ShaderParams* perMeshParams, ShaderParams* perInstanceParams,
				const std::string& meshName, MeshLoadFunc loadFunc, Material* material);
			GraphicsComponent(Entity* owner, RendererTypes type, ShaderParams* params,const std::string& meshName, Material* material);
			~GraphicsComponent();

			std::string getParamsId();
			const std::string& getMeshName() const {
				return meshName_;
			}
			Entity* getEntity() const {
				return owningEntity_;
			}
			ShaderParams* getShaderParams() {
				return instanceParameters_;
			}
			Material* getMaterial() {
				return material_;
			}
			ShaderParams* getPerMeshShaderParams() {
				return meshParameters_;
			}
			const RendererTypes getRendererType() {
				return rtype_;
			}
			MeshLoadFunc& getLoadInfo() {
				return loadFunc_;
			}
			BasicMesh* getMesh() {
				return mesh_;
			}
			void setMesh(BasicMesh* mesh) {
				mesh_ = mesh;
			}
			glm::mat4 getModelmatrix();
		private:
			Entity* owningEntity_;
			RendererTypes rtype_;
			const std::string meshName_;
			BasicMesh* mesh_;
			MeshLoadFunc loadFunc_;
			ShaderParams* instanceParameters_;
			ShaderParams* meshParameters_;
			Material* material_;
		};
	}
}
