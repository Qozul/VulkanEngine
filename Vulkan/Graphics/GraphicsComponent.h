#pragma once
#include "VkUtil.h"
#include "ShaderParams.h"
#include "../Graphics/MeshLoader.h"

namespace QZL {
	namespace Assets {
		class Entity;
	}
	namespace Graphics {
		class GraphicsComponent {
		public:
			GraphicsComponent(Assets::Entity* owner, Graphics::RendererTypes type, ShaderParams* shaderParams, const std::string& meshName, MeshLoaderFunction loadFunc)
				: rtype_(type), owningEntity_(owner), shaderParameters_(shaderParams), meshName_(meshName), loadFunc_(loadFunc), loadFuncOnlyPos_(nullptr) {
				// Component renderer and shader params must agree for valid type casting by the graphics system
				ASSERT(type == shaderParams->getRendererType());
				vertexType_ = VertexType::POSITION_UV_NORMAL;
			}
			GraphicsComponent(Assets::Entity* owner, Graphics::RendererTypes type, ShaderParams* shaderParams, const std::string& meshName, MeshLoaderFunctionOnlyPos loadFunc)
				: rtype_(type), owningEntity_(owner), shaderParameters_(shaderParams), meshName_(meshName), loadFunc_(nullptr), loadFuncOnlyPos_(loadFunc) {
				ASSERT(type == shaderParams->getRendererType());
				vertexType_ = VertexType::POSITION_ONLY;
			}
			~GraphicsComponent();
			const std::string& getMeshName() const {
				return meshName_;
			}
			Assets::Entity* getEntity() const {
				return owningEntity_;
			}
			ShaderParams* getShaderParams() {
				return shaderParameters_;
			}
			const RendererTypes getRendererType() {
				return rtype_;
			}
			MeshLoaderFunction getLoadFunc() {
				return loadFunc_;
			}
			MeshLoaderFunctionOnlyPos getLoadFuncOnlyPos() {
				return loadFuncOnlyPos_;
			}
			const VertexType getVertexType() const {
				return vertexType_;
			}
		private:
			Assets::Entity* owningEntity_; // does not own this object
			RendererTypes rtype_;
			const std::string meshName_;
			MeshLoaderFunction loadFunc_;
			MeshLoaderFunctionOnlyPos loadFuncOnlyPos_;
			VertexType vertexType_;
			ShaderParams* shaderParameters_;
		};
	}
}
