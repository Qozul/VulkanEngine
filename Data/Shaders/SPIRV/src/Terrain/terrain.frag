#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#define USE_LIGHTS_UBO
#include "../common.glsl"
#include "terrain_structs.glsl"

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 inWorldPos;
layout (location = 2) in vec3 inNormal;
layout (location = 3) flat in int instanceIndex;
layout (location = 4) flat in vec3 inCamPos;
layout (location = 5) flat in int inGrass;

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer ParamsData
{
	Params params[];
};
layout(set = COMMON_SET, binding = COMMON_MATERIALS_BINDING) readonly buffer TexIndices
{
	TextureIndices textureIndices[];
};

const float mixThreshold = 0.1;
const float mixThresholdMult2 = mixThreshold * 2.0;

const vec4 fogColour = vec4(0.7, 0.7, 0.8, 1.0);
const vec3 grassColours[3] = vec3[](vec3(0.486, 0.988, 0.0), vec3(0.220, 0.273, 0.060), vec3(0.129, 0.203, 0.016) );

void main() {
	Params parameters = params[SC_PARAMS_OFFSET + instanceIndex];
	TextureIndices texIndices = textureIndices[SC_MATERIAL_OFFSET + instanceIndex];
	
	outPosition = vec4(inWorldPos, 1.0);
	outNormal = vec4(inNormal * 0.5 + 0.5, 90.0);
	
	// Grass
	if (inGrass > -1) {
		outAlbedo = vec4(grassColours[inGrass], 1.0);
	}
	else {
		// Terrain texture
		float heightFactor = clamp(inWorldPos.y / parameters.heights.x, 0.0, 1.0);
		vec4 texColour0 = texture(texSamplers[nonuniformEXT(texIndices.albedoIdx0)], texUV);
		vec4 texColour1 = texture(texSamplers[nonuniformEXT(texIndices.albedoIdx1)], texUV);
		vec4 texColour2 = texture(texSamplers[nonuniformEXT(texIndices.albedoIdx2)], texUV);
		float heightFactors[3];
		
		float zboundary = heightFactor - (parameters.heights.z - mixThreshold);
		float wboundary = heightFactor - (parameters.heights.w - mixThreshold);
		heightFactors[0] = 1.0 - clamp(zboundary / mixThresholdMult2, 0.0, 1.0);
		heightFactors[1] = clamp(zboundary/mixThresholdMult2, 0.0, 1.0) *
			1.0 - clamp(wboundary / mixThresholdMult2, 0.0, 1.0);
		heightFactors[2] = clamp(wboundary / mixThresholdMult2, 0.0, 1.0);
		
		outAlbedo = vec4(texColour0.rgb * heightFactors[0] + texColour1.rgb * heightFactors[1] + texColour2.rgb * heightFactors[2], 1.0);
		vec4 tmpAlbedo = mix(fogColour, outAlbedo, heightFactor * heightFactor);
		float distFactor = (distance(inWorldPos, inCamPos) - 0.1) / 1000.0;
		outAlbedo = vec4(mix(outAlbedo, tmpAlbedo, distFactor * distFactor).rgb, 1.0);
	}
}
