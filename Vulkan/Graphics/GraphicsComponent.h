#pragma once
#include "VkUtil.h"
#include "GraphicsMaster.h"

namespace QZL {
	namespace Assets {
		class Entity;
	}
	namespace Graphics {
		class StaticShaderParams;

		union ShaderParams {
			StaticShaderParams* ssp;
		};

		class GraphicsComponent {
		public:
			GraphicsComponent(Assets::Entity* owner, Graphics::RendererTypes type, const std::string& meshName, ShaderParams shaderParams)
				: rtype_(type), owningEntity_(owner), meshName_(meshName), shaderParameters_(shaderParams) { }
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
		private:
			Assets::Entity* owningEntity_; // does not own this object
			RendererTypes rtype_;
			const std::string meshName_;

			ShaderParams shaderParameters_;
		};
	}
}
