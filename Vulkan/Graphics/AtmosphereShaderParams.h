#pragma once
#include "ShaderParams.h"

namespace QZL {
	namespace Assets {
		struct PrecomputedTextures;
	}
	namespace Graphics {
		struct MaterialAtmosphere;
		class AtmosphereShaderParams : public ShaderParams {
		public:
			AtmosphereShaderParams(Assets::PrecomputedTextures& tex, MaterialAtmosphere& material)
				: textures(tex), material(material) {}
			const RendererTypes getRendererType() const override {
				return RendererTypes::ATMOSPHERE;
			}
			const std::string getParamsId() const override {
				return "";
			}
			MaterialAtmosphere& material;
			Assets::PrecomputedTextures& textures;
		};
	}
}
