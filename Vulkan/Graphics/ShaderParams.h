// Author: Ralph Ridley
// Date: 12/10/19
// Define per draw call params for render objects. Define per instance params for each graphics component.

#pragma once
#include "GraphicsMaster.h"
#include "Material.h"

namespace QZL {
	namespace Graphics {
		struct ShaderParams {
			std::string id;
			ShaderParams(std::string id = "")
				: id(id) { };
			virtual const RendererTypes getRendererType() const = 0;
		};

		// Derived params need internal wrapping struct for correct use and sizing, since subclassing adds size and makes memory allocation in
		// the renderers more complicated.

		struct ParticleShaderParams : public ShaderParams {
			struct Params {
				float textureTileLength;
				glm::vec4 tint;
			} params;

			ParticleShaderParams(float tileLength, glm::vec4 tint)
				: params({ tileLength, tint }) { }

			const RendererTypes getRendererType() const override {
				return RendererTypes::kParticle;
			}
		};

		struct AtmosphereShaderParams : public ShaderParams {
			struct Params {
				glm::vec3 betaRay;
				float betaMie;

				float planetRadius;

				glm::vec3* sunDirection;
				float Hatm;

				glm::vec3* sunIntensity;
				float g;

				glm::vec3 betaOzoneExt;
				float betaMieExt;

				float mieScaleHeight;
				float rayleighScaleHeight;
				
			} params;

			const RendererTypes getRendererType() const override {
				return RendererTypes::kAtmosphere;
			}
		};

		struct StaticShaderParams : public ShaderParams {
			struct Params {
				float diffuseX = 0.7f;
				float diffuseY = 0.7f;
				float diffuseZ = 0.7f;
				float alpha = 1.0f;
				float specularX = 0.7f;
				float specularY = 0.7f;
				float specularZ = 0.7f;
				float specularExponent = 41.0f;
			} params;

			const RendererTypes getRendererType() const override {
				return RendererTypes::kStatic;
			}
		};

		struct TerrainShaderParams : public ShaderParams {
			struct Params {
				glm::vec3 diffuseColour;
				float alpha = 1.0f;
				glm::vec3 specularColour;
				float specularExponent = 1.0f;
			} params;

			TerrainShaderParams(glm::vec3 diffuseColour, glm::vec3 specularColour, float alpha, float specularExponent) {
				params.diffuseColour = diffuseColour;
				params.alpha = alpha;
				params.specularColour = specularColour;
				params.specularExponent = specularExponent;
			}
			const RendererTypes getRendererType() const override {
				return RendererTypes::kTerrain;
			}
		};
	}
}
