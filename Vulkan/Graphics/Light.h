// Author: Ralph Ridley;
// Date: 21/11/19
#pragma once
#include "VkUtil.h"

#define MAX_LIGHTS 250

namespace QZL {
	namespace Graphics {
		struct Light {
			glm::vec3 position;
			float radius;
			glm::vec3 colour;
			float padding;
		};
	}
}