// Author: Ralph Ridley
// Date: 01/11/19
#pragma once
#include "VkUtil.h"
#include "GraphicsTypes.h"

namespace QZL {
	class Entity;
	namespace Graphics {
		struct ShaderParams;
		class Material;
		class RenderObject;
		class GraphicsComponent {
			friend class Entity;
		public:
			GraphicsComponent(Entity* owner, RendererTypes type, ShaderParams* perMeshParams, ShaderParams* perInstanceParams,
				const std::string& meshName, MeshLoadFunc loadFunc, Material* material, bool overrideChecks = false);
			GraphicsComponent(Entity* owner, RendererTypes type, RenderObject* robject, ShaderParams* perInstanceParams = nullptr);
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
			glm::mat4 getModelmatrix();
		private:
			Entity* owningEntity_;
			RendererTypes rtype_;
			const std::string meshName_;
			MeshLoadFunc loadFunc_;
			ShaderParams* instanceParameters_;
			ShaderParams* meshParameters_;
			Material* material_;
		};
	}
}
