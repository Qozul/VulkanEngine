// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

namespace QZL {
	namespace Assets {
		class Entity;
	}
	namespace Graphics {
		struct ShaderParams;
		class Material;
		class RenderObject;
		class GraphicsComponent {
		public:
			GraphicsComponent(Assets::Entity* owner, RendererTypes type, ShaderParams* perMeshParams, ShaderParams* perInstanceParams,
				const std::string& meshName, MeshLoadFunc loadFunc, Material* material);
			GraphicsComponent(Assets::Entity* owner, RendererTypes type, RenderObject* robject, ShaderParams* perInstanceParams = nullptr);
			~GraphicsComponent();

			std::string getParamsId();
			const std::string& getMeshName() const {
				return meshName_;
			}
			Assets::Entity* getEntity() const {
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
			glm::mat4 getModelmatrix();
		private:
			Assets::Entity* owningEntity_;
			RendererTypes rtype_;
			const std::string meshName_;
			MeshLoadFunc loadFunc_;
			ShaderParams* instanceParameters_;
			ShaderParams* meshParameters_;
			Material* material_;
		};
	}
}
