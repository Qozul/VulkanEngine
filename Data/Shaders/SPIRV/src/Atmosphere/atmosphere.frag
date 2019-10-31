#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "./alt_functions.glsl"

layout(constant_id = 0) const float SC_NEAR_Z = 0.1;
layout(constant_id = 1) const float SC_FAR_Z = 1000.0;

const float AP_HEIGHT_ANGLE_OFFSET = 0.3;

layout(location = 0) out vec4 colour;
layout(location = 0) in vec2 pos;
layout(push_constant) uniform Params {
	mat4 inverseViewProj;
	vec4 betaRay; // .w = float betaMie	
	vec4 cameraPosition; // .w = float planetRadius
	vec4 sunDirection; // .w = float Hatm
	vec4 sunIntensity; // .w = float g
} PC;
layout(set = 0, binding = 0) uniform sampler3D scatteringTexture;

// theta is the angle between the direction of the incident light and the direction of the scattered light
float rayleighPhase(float ctheta)
{
	return 0.8 * (1.4 + 0.5 * ctheta * ctheta);
}

// g is in range [-1, 1]
float miePhase(float ctheta, float g)
{
	float g2 = g * g;
	return ((3.0 * (1.0 - g2)) / (2.0 * (2.0 + g2))) * ((1.0 + ctheta * ctheta) / pow(1.0 + g2 - 2.0 * g * ctheta * ctheta, 1.5));
}

void calculateRayleighAndMie(in vec3 V, in vec3 L, in vec3 Z, out vec3 rayleigh, out vec3 mie) 
{
	float height = clamp(length(PC.cameraPosition.xyz), 0.0, PC.sunDirection.w);
	float Cv = dot(V, Z);
	float Cs = dot(L, Z);
	// Fetch rayleigh and mie scattered light
	vec4 scattering = texture(scatteringTexture, vec3(heightToUh(height, PC.sunDirection.w), 
		CvToUv(Cv, height, PC.cameraPosition.w), CsToUs(Cs)));
	rayleigh = scattering.rgb;
	
	mie = extractMieFromScattering(scattering, PC.betaRay.w, PC.betaRay.xyz);
	
	// Apply phase functions
	float ctheta = dot(V, L);
	rayleigh *= rayleighPhase(ctheta);
	mie *= miePhase(ctheta, PC.sunIntensity.w);
}

void main() 
{	
	vec3 Z = vec3(0.0, 1.0, 0.0);
	//vec3 Z = normalize(PC.cameraPosition.xyz);
	vec3 L = normalize(PC.sunDirection.xyz);
	vec3 V = normalize((PC.inverseViewProj * vec4(pos * 2.0 - 1.0, 1.0, 1.0)).xyz);
	vec3 rayleigh;
	vec3 mie;
	calculateRayleighAndMie(V, L, Z, rayleigh, mie);
		
	colour = vec4(rayleigh + mie, 1.0) * 2.0;// * vec4(PC.sunIntensity.xyz, 1.0);
	colour = colour / (colour + vec4(1.0, 1.0, 1.0, 0.0));
	colour.rgb = pow(colour.rgb, vec3(1.0/2.2));
}