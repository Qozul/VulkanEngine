#pragma once
#include "ShaderParams.h"

namespace QZL {
	/*namespace Assets {
		struct PrecomputedTextures;
	}*/
	namespace Game {
		class SunScript;
		struct PrecomputedTextures;
	}
	namespace Graphics {
		struct MaterialAtmosphere;
		class AtmosphereShaderParams : public ShaderParams {
		public:
			AtmosphereShaderParams(Game::PrecomputedTextures& tex, MaterialAtmosphere& material, Game::SunScript* sunScript)
				: textures(tex), material(material), sunScript(sunScript) {}
			const RendererTypes getRendererType() const override {
				return RendererTypes::ATMOSPHERE;
			}
			const std::string getParamsId() const override {
				return "";
			}
			MaterialAtmosphere& material;
			Game::PrecomputedTextures& textures;
			Game::SunScript* sunScript;
		};
	}
}
