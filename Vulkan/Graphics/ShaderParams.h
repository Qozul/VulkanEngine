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
				return RendererTypes::PARTICLE;
			}
		};

		struct AtmosphereShaderParams : public ShaderParams {
			struct Params {
				glm::vec3 betaRay;
				float betaMie;

				glm::vec3 cameraPosition;
				float planetRadius;

				glm::vec3* sunDirection;
				float Hatm;

				glm::vec3* sunIntensity;
				float g;
			} params;

			const RendererTypes getRendererType() const override {
				return RendererTypes::ATMOSPHERE;
			}
		};

		struct StaticShaderParams : public ShaderParams {
			struct Params {
				float diffuseX, diffuseY, diffuseZ;
				float alpha;
				float specularX, specularY, specularZ;
				float specularExponent;
				float padding0, padding1;
			} params;

			const RendererTypes getRendererType() const override {
				return RendererTypes::STATIC;
			}
		};

		struct TerrainShaderParams : public ShaderParams {
			struct Params {
				glm::vec3 diffuseColour;
				float alpha;
				glm::vec3 specularColour;
				float specularExponent;
			} params;

			TerrainShaderParams(glm::vec3 diffuseColour, glm::vec3 specularColour, float alpha, float specularExponent) {
				params.diffuseColour = diffuseColour;
				params.alpha = alpha;
				params.specularColour = specularColour;
				params.specularExponent = specularExponent;
			}
			const RendererTypes getRendererType() const override {
				return RendererTypes::TERRAIN;
			}
		};
	}
}
