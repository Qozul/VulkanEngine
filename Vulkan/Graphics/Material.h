#pragma once
#include "VkUtil.h"
#include "../Assets/AtmosphereParameters.h"
#include <fstream>
#include <sstream>

namespace QZL {
	namespace Graphics {
		class TextureSampler;

		// A material is a simple wrapper around the requirements of a renderer.
		// It will usually consist of a set of textures and possibly some additional data.
		// The data should be accessible via getMaterialData, and will likely correspond to a data struct defined by the subclass.
		// A material has its own data set with the textures held in immutable samplers at bindings of a set.
		class Material {
		public:
			Material(const std::string materialFileName)
				: materialFileName_(materialFileName), textureSet_(VK_NULL_HANDLE) {}

			VkDescriptorSet getTextureSet() {
				return textureSet_;
			}
		protected:
			

			virtual void loadMaterial() = 0;

			VkDescriptorSet textureSet_;
			const std::string materialFileName_;
		};

		class ParticleMaterial : public Material {
		public:
			ParticleMaterial(const std::string materialFileName)
				: Material(materialFileName) { }
		protected:
			void loadMaterial() override {
				std::ifstream requirementsFile("../descriptor-requirements.txt");
				ASSERT(requirementsFile.is_open());
				std::string type, diffuseTexture;
				requirementsFile >> type >> diffuseTexture;
				requirementsFile.close();

				ASSERT(type == "PARTICLE");
				// todo load texture from texture manager.
				//      call this from texture manager.
				//      and store the material so it can be reused.
				//      Add this to GraphicsComponent using Material interface.
			}
		};

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
		struct MaterialAtmosphere {
			glm::vec3 betaRay;
			float betaMie;

			glm::vec3 cameraPosition;
			float planetRadius;

			glm::vec3 sunDirection;
			float Hatm;

			glm::vec3 sunIntensity;
			float g;
		};
		struct MaterialParticle {
			float textureTileLength;
			glm::vec4 tint;
			TextureSampler* texture;
		};
	}
}
