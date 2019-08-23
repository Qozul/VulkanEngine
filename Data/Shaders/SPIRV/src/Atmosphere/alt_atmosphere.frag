#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "./alt_functions.glsl"

layout(location = 0) out vec4 color;
layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec2 uvcoords;
layout(push_constant) uniform Params {
	vec3 betaRay;
	float betaMie;
	
	vec3 cameraPosition;
	float planetRadius;
	
	vec3 sunDirection;
	float Hatm;
	
	vec3 sunIntensity;
	float g;
} PC;
layout(set = 0, binding = 1) uniform sampler3D scatteringTexture;

// theta is the angle between the direction of the incident light and the direction of the scattered light
float rayleighPhase(float ctheta)
{
	return 0.8 * (1.4 + 0.5 * ctheta * ctheta);
}

// g is in range [-1, 1]
float miePhase(float ctheta, float g)
{
	float g2 = g * g;
	float cos2Theta = ctheta * ctheta;
	return ((3.0 * (1.0 - g2)) / (2.0 * (2.0 + g2))) * ((1.0 + ctheta) / pow(1.0 + g2 - 2.0 * g * ctheta, 1.5));
}

void main() 
{
	//vec3 zenith = normalize(cameraPosition.xyz);
	vec3 Z = vec3(0.0, 1.0, 0.0);
	vec3 V = normalize(worldPosition - PC.cameraPosition);
	vec3 L = normalize(PC.sunDirection);
	float height = clamp(length(PC.cameraPosition), 0.0, PC.Hatm);
	float Cv = dot(V, Z);
	float Cs = dot(L, Z);
	// Fetch rayleigh and mie scattered light
	vec4 scattering = texture(scatteringTexture, vec3(heightToUh(height, PC.Hatm), 
		CvToUv(Cv, height, PC.planetRadius), CsToUs(Cs)));
	vec3 rayleigh = scattering.rgb;
	
	vec3 mie = extractMieFromScattering(scattering, PC.betaMie, PC.betaRay);
	
	// Apply phase functions
	float ctheta = dot(V, L);
	rayleigh *= rayleighPhase(ctheta);
	mie *= miePhase(ctheta, PC.g);
	
	color = vec4(rayleigh + mie, 1.0) * vec4(PC.sunIntensity, 1.0);
	color = color / (color + vec4(1.0, 1.0, 1.0, 0.0));
	color.rgb = pow(color.rgb, vec3(1.0/2.2));
}
