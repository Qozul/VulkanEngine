// Author: Ralph Ridley
// Date: 01/11/19
#pragma once

namespace QZL {
	namespace Graphics {
		enum class RendererTypes {
			STATIC, TERRAIN, ATMOSPHERE, PARTICLE, POST_PROCESS
		};

		enum class RenderPassTypes : size_t {
			GEOMETRY = 0, POST_PROCESS = 1
		};
	}
}
