#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "./alt_functions.glsl"

layout(location = 0) out vec4 color;
layout(location = 0) in vec2 pos;
layout(push_constant) uniform Params {
	mat4 inverseViewProj;
	vec4 betaRay; // .w = float betaMie	
	vec4 cameraPosition; // .w = float planetRadius
	vec4 sunDirection; // .w = float Hatm
	vec4 sunIntensity; // .w = float g
} PC;
layout(set = 0, binding = 0) uniform sampler3D scatteringTexture;
layout(binding = 1) uniform sampler2D geometryColourBuffer;
layout(binding = 2) uniform sampler2D geometryDepthBuffer;

void main() 
{
	//vec3 zenith = normalize(cameraPosition.xyz);
	vec3 Z = vec3(0.0, 1.0, 0.0);
	vec3 V = normalize((PC.inverseViewProj * vec4(pos * 2.0 - 1.0, 1.0, 1.0)).xyz);
	vec3 L = normalize(PC.sunDirection.xyz);
	float height = clamp(PC.cameraPosition.y, 0.0, PC.sunDirection.w);
	float Cv = dot(V, Z);
	float Cs = dot(L, Z);
	// Fetch rayleigh and mie scattered light
	vec4 scattering = texture(scatteringTexture, vec3(heightToUh(height, PC.sunDirection.w), 
		CvToUv(Cv, height, PC.cameraPosition.w), CsToUs(Cs)));
	vec3 rayleigh = scattering.rgb;
	
	vec3 mie = extractMieFromScattering(scattering, PC.betaRay.w, PC.betaRay.xyz);
	
	// Apply phase functions
	float ctheta = dot(V, L);
	rayleigh *= rayleighPhase(ctheta);
	mie *= miePhase(ctheta, PC.sunIntensity.w);
	
	color = vec4(rayleigh + mie, 1.0) * vec4(PC.sunIntensity.xyz, 1.0);
	color = color / (color + vec4(1.0, 1.0, 1.0, 0.0));
	color.rgb = pow(color.rgb, vec3(1.0/2.2));
	
	color += texture(geometryColourBuffer, pos);
}
