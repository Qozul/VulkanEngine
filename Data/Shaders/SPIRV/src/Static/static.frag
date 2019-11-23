#version 450
#extension GL_GOOGLE_include_directive : enable
#define USE_LIGHTS_UBO
#include "../common.glsl"

struct Params {
    mat4 model;
	vec4 diffuseColour;
	vec4 specularColour;
};

struct TextureIndices {
	uint diffuseIdx;
	uint inNormalMapIdx;
};

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inWorldPos;
layout(location = 3) flat in int inInstanceIndex;
layout(location = 4) in vec4 inShadowCoord;
layout(location = 5) flat in uint inShadowMapIdx;
layout(location = 6) flat in vec3 inCamPos;

layout(location = 0) out vec4 outColour;

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer MaterialData
{
	Params params[];
};

layout(set = COMMON_SET, binding = COMMON_MATERIALS_BINDING) readonly buffer TextureIndexBuffer
{
	TextureIndices texIndices[];
};

void main() {
	Params parameters = params[SC_PARAMS_OFFSET + inInstanceIndex];
	float lambert, sFactor;
	calculatePhongShading(inWorldPos, lights[0].position, inCamPos, lights[0].radius, inNormal, parameters.specularColour.w, lambert, sFactor);
	
	TextureIndices texIdxs = texIndices[SC_MATERIAL_OFFSET + inInstanceIndex];
	vec4 texColour = texture(texSamplers[nonuniformEXT(texIdxs.diffuseIdx)], inUV);
	vec4 texColour2 = texture(texSamplers[nonuniformEXT(texIdxs.inNormalMapIdx)], inUV);
	
	vec3 ambient = texColour.rgb * lights[0].padding; // For the main light the padding holds ambience
	vec3 diffuse = texColour.rgb * lights[0].colour * lambert;
	vec3 specular = lights[0].colour * sFactor;

	float shadow = projectShadow(inShadowCoord / inShadowCoord.w, vec2(0.0), inShadowMapIdx);
	outColour = vec4(ambient + diffuse + specular, 1.0);
}
