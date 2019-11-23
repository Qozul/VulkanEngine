#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
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

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer MaterialData
{
	Params params[];
};

layout(set = COMMON_SET, binding = COMMON_MATERIALS_BINDING) readonly buffer TextureIndexBuffer
{
	TextureIndices texIndices[];
};

void main() 
{
	Params parameters = params[SC_PARAMS_OFFSET + inInstanceIndex];
	outPosition = vec4(inWorldPos, 1.0);
	outNormal = vec4(inNormal * 0.5 + 0.5, parameters.specularColour.w);
	TextureIndices texIdxs = texIndices[SC_MATERIAL_OFFSET + inInstanceIndex];
	outAlbedo = texture(texSamplers[nonuniformEXT(texIdxs.diffuseIdx)], inUV);
}
