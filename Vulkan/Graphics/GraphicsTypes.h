#pragma once

namespace QZL {
	namespace Graphics {
		enum class RendererTypes {
			STATIC, TERRAIN, ATMOSPHERE
		};

		enum class RenderPassTypes : size_t {
			GEOMETRY = 0, POST_PROCESS = 1
		};
	}
}
