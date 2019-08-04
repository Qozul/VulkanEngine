#pragma once
#include "VkUtil.h"

namespace QZL {
	namespace Graphics {
		struct MaterialStatic {
			// 32 bytes
			float diffuseX, diffuseY, diffuseZ; // Mixed in with texture
			float alpha; // Sets maximum alpha
			float specularX, specularY, specularZ; // Can create tinting
			float specularExponent; // The intesity of the colour
			uint32_t diffuseTextureIndex; // Only used if descriptor indexing is enabled
			uint32_t normalMapIndex; // Only used if descriptor indexing is enabled
			float padding0, padding1;
			MaterialStatic(glm::vec3 diffuseColour, glm::vec3 specularColour, float alpha, float specExponent) 
				: diffuseX(diffuseColour.x), diffuseY(diffuseColour.y), diffuseZ(diffuseColour.z), alpha(alpha),
				  specularX(specularColour.x), specularY(specularColour.y), specularZ(specularColour.z), specularExponent(specExponent),
				diffuseTextureIndex(0), normalMapIndex(0), padding0(0.0f), padding1(0.0f) { }
			MaterialStatic(glm::vec3 diffuseColour, glm::vec3 specularColour, float alpha, float specExponent, 
				uint32_t diffuseIndex, uint32_t normalMapIndex)
				: diffuseX(diffuseColour.x), diffuseY(diffuseColour.y), diffuseZ(diffuseColour.z), alpha(alpha),
				specularX(specularColour.x), specularY(specularColour.y), specularZ(specularColour.z), specularExponent(specExponent),
				diffuseTextureIndex(diffuseIndex), normalMapIndex(normalMapIndex), padding0(0.0f), padding1(0.0f) { }
		};
	}
}
