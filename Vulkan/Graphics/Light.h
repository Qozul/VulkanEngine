// Author: Ralph Ridley;
// Date: 21/11/19
#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Graphics {
		struct DirectionalLight {
			glm::vec4 direction; // ambient extracted from w components
			glm::vec4 diffuse;
			glm::vec4 specular;
		};

		struct PointLight {
			glm::vec3 position;
			float constant;
			glm::vec3 diffuse;
			float linear;
			glm::vec3 specular;
			float quadratic;
		};

		struct SpotLight {
			glm::vec3 position;
			float padding;
			glm::vec3 direction;
			float angle;
		};
	}
}
