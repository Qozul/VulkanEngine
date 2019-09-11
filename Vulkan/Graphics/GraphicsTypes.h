#pragma once

namespace QZL {
	namespace Graphics {
		enum class RendererTypes {
			STATIC, TERRAIN, ATMOSPHERE, PARTICLE
		};

		enum class RenderPassTypes : size_t {
			GEOMETRY = 0, POST_PROCESS = 1
		};
	}
}
