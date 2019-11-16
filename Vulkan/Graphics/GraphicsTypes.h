// Author: Ralph Ridley
// Date: 01/11/19
#pragma once

namespace QZL {
	namespace Graphics {
		enum class RendererTypes {
			kStatic,
			kTerrain,
			kAtmosphere,
			kParticle,
			kPostProcess,
			kNone
		};

		enum class RenderPassTypes : size_t {
			kComputePrePass,
			kGeometry,
			kPostProcess
		};
	}
}
