#pragma once
#include "ShaderParams.h"

namespace QZL {
	namespace Graphics {
		class AtmosphereShaderParams : public ShaderParams {
		public:
			const RendererTypes getRendererType() const override {
				return RendererTypes::ATMOSPHERE;
			}
			const std::string getParamsId() const override {
				return "";
			}
		};
	}
}
