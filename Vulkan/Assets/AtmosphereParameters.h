#pragma once
#include "../Graphics/VkUtil.h"

namespace QZL
{
	namespace Assets {
		struct AtmosphereParameters {

			glm::vec3 betaRay;
			float planetRadius;

			glm::vec3 betaOzoneExt;
			float rayleighScaleHeight;

			float betaMie;
			float betaMieExt;
			float Hatm;
			float mieScaleHeight;
		};
	}
}
