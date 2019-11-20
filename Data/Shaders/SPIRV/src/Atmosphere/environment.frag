#version 450
#define OVERRIDE_TEX_SAMPLERS
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

struct Params {
	mat4 inverseViewProj;
	vec4 betaRay; // float betaMie;
	vec4 sunDirection; // float planetRadius;
	vec4 sunIntensity;//  float Hatm;
	float g;
	uint scatteringIdx;
	float padding0;
	float padding1;
};

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;

layout (location = 0) out vec4 colour;

layout (location = 0) in vec2 pos;
layout (location = 1) flat in vec4 cameraPos;

layout(set = 0, binding = 1) readonly buffer ShaderParamsData
{
	Params params[];
};

layout(set = GLOBAL_SET, binding = ENVIORNMENT_SAMPLER_BINDING) uniform samplerCube environmentMap;

void main()
{	
	Params parameters = params[SC_PARAMS_OFFSET];
	vec3 V = normalize((parameters.inverseViewProj * vec4(pos * 2.0 - 1.0, 1.0, 1.0)).xyz);
		
	colour = texture(environmentMap, V);
}