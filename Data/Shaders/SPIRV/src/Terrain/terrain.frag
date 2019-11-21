#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"
#include "terrain_structs.glsl"

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout(location = 0) out vec4 fragColor;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec3 normal;
layout (location = 3) flat in int instanceIndex;
layout (location = 4) in vec4 shadowCoord;
layout (location = 5) flat in uint shadowMapIdx;
layout (location = 6) in vec3 Fext;
layout (location = 7) in vec3 Lin;

layout(set = GLOBAL_SET, binding = LIGHT_UBO_BINDING) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

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

const vec4 fogColour = vec4(0.7, 0.7, 0.7, 1.0);

void main() {
	Params param = params[SC_PARAMS_OFFSET + instanceIndex];
	TextureIndices texIndices = textureIndices[SC_MATERIAL_OFFSET + instanceIndex];	
	float lambert;
	float sFactor;
	calculatePhongShading(worldPos, lightPositions[0].xyz, cameraPosition.xyz, normal, 49.0,  lambert, sFactor);
	
	float heightFactor = clamp(worldPos.y / param.heights.x, 0.0, 1.0);
	vec4 texColour0 = texture(texSamplers[nonuniformEXT(texIndices.albedoIdx0)], texUV);
	vec4 texColour1 = texture(texSamplers[nonuniformEXT(texIndices.albedoIdx1)], texUV);
	vec4 texColour2 = texture(texSamplers[nonuniformEXT(texIndices.albedoIdx2)], texUV);
	float heightFactors[3];
	
	float zboundary = heightFactor - (param.heights.z - mixThreshold);
	float wboundary = heightFactor - (param.heights.w - mixThreshold);
	heightFactors[0] = 1.0 - clamp(zboundary / mixThresholdMult2, 0.0, 1.0);
	heightFactors[1] = clamp(zboundary/mixThresholdMult2, 0.0, 1.0) *
		1.0 - clamp(wboundary / mixThresholdMult2, 0.0, 1.0);
	heightFactors[2] = clamp(wboundary / mixThresholdMult2, 0.0, 1.0);
	
	vec3 finalAlbedo = texColour0.rgb * heightFactors[0] + texColour1.rgb * heightFactors[1] + texColour2.rgb * heightFactors[2];
	vec3 ambient = finalAlbedo.rgb * ambientColour.xyz;
	vec3 specular = finalAlbedo.rgb * sFactor;
	vec3 diffuse = max(finalAlbedo.rgb * lambert, ambient);
	
	float shadow = projectShadow(shadowCoord / shadowCoord.w, vec2(0.0), shadowMapIdx);
	fragColor = vec4((diffuse + specular) * shadow, 1.0);
	vec4 tmpFragColor0 = mix(vec4(Lin * 0.001, 1.0), fragColor, heightFactor * heightFactor);
	float distFactor = (distance(worldPos, cameraPosition.xyz) - 0.1) / 1000.0;
	fragColor = mix(fragColor, tmpFragColor0, distFactor * distFactor);
	reinhardTonemap(fragColor);
}
