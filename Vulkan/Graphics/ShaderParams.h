// Author: Ralph Ridley
// Date: 12/10/19
// Define per draw call params for render objects. Define per instance params for each graphics component.

#pragma once
#include "GraphicsMaster.h"
#include "Material.h"

namespace QZL {
	namespace Graphics {
		struct ShaderParams {
			static const size_t shaderParamsLUT[(size_t)RendererTypes::kNone];
		};
		struct StaticShaderParams : ShaderParams {
			float diffuseX = 0.7f;
			float diffuseY = 0.7f;
			float diffuseZ = 0.7f;
			float alpha = 1.0f;
			float specularX = 0.7f;
			float specularY = 0.7f;
			float specularZ = 0.7f;
			float specularExponent = 41.0f;
			glm::mat4 model;
			//uint32_t materialIdx;
		};
		struct TerrainShaderParams : ShaderParams {
			glm::vec3 albedoCol;
			float alpha = 1.0f;
			glm::vec3 specularCol;
			float specularExponent = 1.0f;
			glm::mat4 model;
			//uint32_t materialIdx;
			TerrainShaderParams(glm::vec3 albedo, glm::vec3 specular, float alpha, float specExponent)
				: albedoCol(albedo), alpha(alpha), specularCol(specular), specularExponent(specExponent) { }
		};
		struct AtmosphereShaderParams : ShaderParams {
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
			//uint32_t materialIdx;
		};
		struct ParticleShaderParams : ShaderParams {
			float textureTileLength;
			glm::vec4 tint;
			//uint32_t materialIdx;
			ParticleShaderParams(float texTileLength, glm::vec4 tint)
				: textureTileLength(texTileLength), tint(tint) { }
		};
	}
}
