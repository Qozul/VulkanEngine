#pragma once
#include "GraphicsMaster.h"
#include "Material.h"

namespace QZL {
	namespace Graphics {
		class ShaderParams {
		public:
			virtual const RendererTypes getRendererType() const = 0;
			virtual const std::string getParamsId() const = 0;
		};
	}
}
