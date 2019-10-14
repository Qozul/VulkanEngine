#pragma once
#include "ShaderParams.h"
/*
namespace QZL {
	namespace Game {
		class GameMaster;
	}
	namespace Graphics {
		class StaticShaderParams : public ShaderParams {
			friend class GraphicsComponent;
			friend class TexturedRenderer;
		public:
			StaticShaderParams(const std::string& diffuseName, const std::string& normalMapName, MaterialStatic material)
				: diffuse_(diffuseName), normalMap_(normalMapName), material_(material) { }

			const RendererTypes getRendererType() const override {
				return RendererTypes::STATIC;
			}
			const std::string getParamsId() const override {
				return diffuse_ + "." + normalMap_;
			}
			const std::string& getDiffuseName() const {
				return diffuse_;
			}
			const std::string& getNormalMapName() const {
				return normalMap_;
			}
			MaterialStatic& getMaterial() {
				return material_;
			}
		private:
			const std::string diffuse_;
			const std::string normalMap_;
			MaterialStatic material_;
		};
	}
}*/