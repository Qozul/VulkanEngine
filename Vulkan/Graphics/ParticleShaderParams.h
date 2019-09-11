#pragma once
#include "ShaderParams.h"

namespace QZL {
	namespace Graphics {
		class ParticleShaderParams : public ShaderParams {
		public:
			ParticleShaderParams(MaterialParticle& material)
				: material(material) {}
			const RendererTypes getRendererType() const override {
				return RendererTypes::PARTICLE;
			}
			const std::string getParamsId() const override {
				return "";
			}
			MaterialParticle& material;
		};
	}
}
