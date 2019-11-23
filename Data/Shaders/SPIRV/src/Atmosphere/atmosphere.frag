#version 450
#define OVERRIDE_TEX_SAMPLERS
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"
#include "./alt_functions.glsl"

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

layout(set = GLOBAL_SET, binding = SAMPLER_ARRAY_BINDING) uniform sampler3D texSamplers[];

// theta is the angle between the direction of the incident light and the direction of the scattered light
float rayleighPhase(float ctheta)
{
	return 0.8 * (1.4 + 0.5 * ctheta);
}

// g is in range [-1, 1]
float miePhase(float ctheta, float g)
{
	float g2 = g * g;
	float c2theta = ctheta * ctheta;
	return ((3.0 * (1.0 - g2)) / (2.0 * (2.0 + g2))) * ((1.0 + c2theta) / pow(1.0 + g2 - 2.0 * g * c2theta, 1.5));
}

void calculateRayleighAndMie(in vec3 V, in vec3 L, in vec3 Z, in Params parameters, out vec3 rayleigh, out vec3 mie) 
{
	float height = clamp(length(cameraPos.y), 0.0, parameters.sunIntensity.w);
	float Cv = dot(V, Z);
	float Cs = dot(L, Z);
	// Fetch rayleigh and mie scattered light
	vec4 scattering = texture(texSamplers[nonuniformEXT(parameters.scatteringIdx)], vec3(heightToUh(height, parameters.sunIntensity.w), 
		CvToUv(Cv, height, parameters.sunDirection.w), CsToUs(Cs)));
	rayleigh = scattering.rgb;
	
	mie = extractMieFromScattering(scattering, parameters.betaRay.w, parameters.betaRay.xyz);
	
	// Apply phase functions
	float ctheta = dot(V, L);
	rayleigh *= rayleighPhase(ctheta);
	mie *= miePhase(ctheta, parameters.g);
}

void main()
{	
	Params parameters = params[SC_PARAMS_OFFSET];
	vec3 Z = vec3(0.0, 1.0, 0.0);
	vec3 L = normalize(parameters.sunDirection.xyz);
	vec4 Vclip = (parameters.inverseViewProj * vec4(pos * 2.0 - 1.0, 1.0, 1.0));
	vec3 V = normalize(Vclip.xyz / Vclip.w - cameraPos.xyz);
	vec3 rayleigh;
	vec3 mie;
	calculateRayleighAndMie(V, L, Z, parameters, rayleigh, mie);
		
	colour = vec4(rayleigh + mie, 1.0) * vec4(parameters.sunIntensity.xyz, 1.0);
	colour = colour / (colour + vec4(1.0, 1.0, 1.0, 0.0));
	colour.rgb = pow(colour.rgb, vec3(1.0/2.2));
	float Cs = dot(L, Z);
	colour.a = Cs < 0 ? Cs + 0.8 : Cs + 0.9;
}