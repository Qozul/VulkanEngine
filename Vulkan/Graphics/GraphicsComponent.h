#pragma once
#include "VkUtil.h"
#include "ShaderParams.h"
#include "../Graphics/MeshLoader.h"
#include "RenderObject.h"

namespace QZL {
	namespace Assets {
		class Entity;
	}
	namespace Graphics {
		class GraphicsComponent {
		public:
			GraphicsComponent(Assets::Entity* owner, RendererTypes type, ShaderParams* perMeshParams, ShaderParams* perInstanceParams,
				const std::string& meshName, MeshLoadingInfo loadInfo, Material* material)
				: rtype_(type), owningEntity_(owner), meshParameters_(perMeshParams), instanceParameters_(perInstanceParams), 
				  meshName_(meshName), loadInfo_(loadInfo), material_(material) {
				if (material != nullptr) {
					ASSERT(type == material->getRendererType());
				}
				if (perInstanceParams != nullptr) {
					ASSERT(type == perInstanceParams->getRendererType());
				}
				if (perMeshParams != nullptr) {
					ASSERT(type == perMeshParams->getRendererType());
				}
			}
			GraphicsComponent(Assets::Entity* owner, RendererTypes type, RenderObject* robject, ShaderParams* perInstanceParams = nullptr)
				: rtype_(type), owningEntity_(owner), meshParameters_(robject->getParams()), instanceParameters_(perInstanceParams),
				meshName_(robject->getMeshName()), loadInfo_(), material_(robject->getMaterial()) {
				if (robject->getMaterial() != nullptr) {
					ASSERT(type == robject->getMaterial()->getRendererType());
				}
				if (perInstanceParams != nullptr) {
					ASSERT(type == perInstanceParams->getRendererType());
				}
				if (robject->getParams() != nullptr) {
					ASSERT(type == robject->getParams()->getRendererType());
				}
			}

			~GraphicsComponent();
			const std::string& getMeshName() const {
				return meshName_;
			}
			Assets::Entity* getEntity() const {
				return owningEntity_;
			}
			ShaderParams* getShaderParams() {
				return instanceParameters_;
			}
			std::string getParamsId() {
				return meshParameters_ == nullptr ? "" : meshParameters_->id;
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
			MeshLoadingInfo& getLoadInfo() {
				return loadInfo_;
			}
			glm::mat4 getModelmatrix();
		private:
			Assets::Entity* owningEntity_;
			RendererTypes rtype_;
			const std::string meshName_;
			MeshLoadingInfo loadInfo_;
			ShaderParams* instanceParameters_;
			ShaderParams* meshParameters_;
			Material* material_;
		};
	}
}
