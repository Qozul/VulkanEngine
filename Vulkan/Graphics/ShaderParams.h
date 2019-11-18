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
			glm::mat4 model;
			float diffuseX = 0.7f;
			float diffuseY = 0.7f;
			float diffuseZ = 0.7f;
			float alpha = 1.0f;
			float specularX = 0.7f;
			float specularY = 0.7f;
			float specularZ = 0.7f;
			float specularExponent = 41.0f;
			//uint32_t materialIdx;
		};
		struct TerrainShaderParams : ShaderParams {
			glm::mat4 model;
			glm::vec3 albedoCol;
			float alpha = 1.0f;
			glm::vec3 specularCol;
			float specularExponent = 1.0f;
			/*float distanceFarMinusClose = 300.0f;
			float closeDistance = 50.0f;
			float patchRadius = 40.0f;
			float maxTessellationWeight = 4.0f;
			std::array<glm::vec4, 6> frustumPlanes;*/
			//uint32_t materialIdx;
			TerrainShaderParams(glm::vec3 albedo, glm::vec3 specular, float alpha, float specExponent)
				: albedoCol(albedo), alpha(alpha), specularCol(specular), specularExponent(specExponent) { }
		};
		struct AtmosphereShaderParams : ShaderParams {
			glm::mat4 inverseViewProj;
			glm::vec3 betaRay;
			float betaMie;
			glm::vec3 sunDirection;
			float planetRadius;
			glm::vec3 sunIntensity;
			float Hatm;
			float g;
			uint32_t scatteringIdx;
		};
		struct ParticleShaderParams : ShaderParams {
			glm::mat4 modelMatrix;
			glm::vec4 tint;
			//uint32_t materialIdx;
			ParticleShaderParams(float texTileLength, glm::vec3 tint) : tint(tint, texTileLength) { }
			ParticleShaderParams(glm::mat4 model, glm::vec4 tint) : modelMatrix(model), tint(tint) { }
		};
	}
}
