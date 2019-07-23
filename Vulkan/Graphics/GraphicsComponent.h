#pragma once
#include "VkUtil.h"
#include "GraphicsMaster.h"
#include "../Graphics/MeshLoader.h"

namespace QZL {
	namespace Assets {
		class Entity;
	}
	namespace Graphics {
		class StaticShaderParams;
		class TerrainShaderParams;

		union ShaderParams {
			StaticShaderParams* staticSP;
			TerrainShaderParams* terrainSP;
		};

		class GraphicsComponent {
		public:
			GraphicsComponent(Assets::Entity* owner, Graphics::RendererTypes type, ShaderParams shaderParams, const std::string& meshName, MeshLoaderFunction loadFunc)
				: rtype_(type), owningEntity_(owner), shaderParameters_(shaderParams), meshName_(meshName), loadFunc_(loadFunc) { }
			~GraphicsComponent();
			const std::string& getMeshName() const {
				return meshName_;
			}
			Assets::Entity* getEntity() const {
				return owningEntity_;
			}
			const ShaderParams& getShaderParams() {
				return shaderParameters_;
			}
			const RendererTypes getRendererType() {
				return rtype_;
			}
			MeshLoaderFunction getLoadFunc() {
				return loadFunc_;
			}
		private:
			Assets::Entity* owningEntity_; // does not own this object
			RendererTypes rtype_;
			const std::string meshName_;
			MeshLoaderFunction loadFunc_;

			ShaderParams shaderParameters_;
		};
	}
}
