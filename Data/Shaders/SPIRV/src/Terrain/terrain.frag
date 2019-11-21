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

void main() {
	Params param = params[SC_PARAMS_OFFSET + instanceIndex];
	TextureIndices texIndices = textureIndices[SC_MATERIAL_OFFSET + instanceIndex];	
	float lambert;
	float sFactor;
	calculatePhongShading(worldPos, lightPositions[0].xyz, cameraPosition.xyz, normal, 49.0,  lambert, sFactor);
	
	vec4 texColour = texture(texSamplers[nonuniformEXT(texIndices.diffuseIdx)], texUV);
	
	vec3 ambient = texColour.rgb * ambientColour.xyz;
	vec3 diffuse = max(texColour.rgb * param.diffuseColour.xyz * lambert, ambient);
	vec3 specular = param.specularColour.xyz * sFactor;
	
	float shadow = projectShadow(shadowCoord / shadowCoord.w, vec2(0.0), shadowMapIdx);
	fragColor = vec4((diffuse + specular) * shadow, 1.0);
	reinhardTonemap(fragColor);
}
