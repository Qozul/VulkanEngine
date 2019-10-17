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
layout(set = 1, binding = 0) uniform sampler3D scatteringTexture;
layout(set = 0,binding = 0) uniform sampler2D geometryColourBuffer;
layout(set = 0, binding = 1) uniform sampler2D geometryDepthBuffer;

float linearizeDepth(float depth)
{
  return (2.0 * SC_NEAR_Z) / (SC_FAR_Z + SC_NEAR_Z - depth * (SC_FAR_Z - SC_NEAR_Z));
}

// theta is the angle between the direction of the incident light and the direction of the scattered light
float rayleighPhase(float ctheta)
{
	return 0.8 * (1.4 + 0.5 * ctheta * ctheta);
}

// g is in range [-1, 1]
float miePhase(float ctheta, float g)
{
	float g2 = g * g;
	return ((3.0 * (1.0 - g2)) / (2.0 * (2.0 + g2))) * ((1.0 + ctheta) / pow(1.0 + g2 - 2.0 * g * ctheta, 1.5));
}

// theta is the angle between the direction of the incident light and the direction of the scattered light
vec3 apRayleighPhase(float ctheta)
{
	return (3.0 / (16.0 * PI)) * PC.betaRay.xyz * (1.0 + ctheta * ctheta);
}

// g is in range [-1, 1]
float apMiePhase(float ctheta, float g)
{
	float g2 = g * g;
	return (1.0 / (4.0 * PI)) * PC.betaRay.w * pow(pow(1.0 - g, 2.0) / (1.0 + g2 - 2.0 * g * ctheta), 1.5);
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

vec3 rotateVectorYZ(vec3 v, float ctheta, float stheta)
{
	return vec3(ctheta * v.x - stheta * v.z, v.y, stheta * v.x + ctheta * v.z);
}

// For areas of the screen where there is no terrain or other things blocking the view ray, calculate the
// colour of the atmosphere. NOTE: this is only intended for use inside the atmosphere.
// Any fragment with depth < 1.0 must have some geometry covering it. Just take the colour of the sky passing through
// it and mix with geometry colour based on the depth.
void main() 
{
	float depth = texture(geometryDepthBuffer, pos).r;
	depth = clamp(linearizeDepth(depth), 0.0, 1.0);
	
	vec3 Z = vec3(0.0, 1.0, 0.0);
	vec3 L = normalize(PC.sunDirection.xyz);
	
	if (depth < 1.0) {
		vec3 V = normalize((PC.inverseViewProj * vec4(pos * 2.0 - 1.0, 1.0, 1.0)).xyz);
		float c = dot(V, Z);
		// Some view rays to the terrain look below the sky where there is no colour (occluded by planet).
		// Inverting the y component makes it always sample above the horizon for its colour.
		V.y = c < 0.0 ? -V.y : V.y;
		// The horizon ring gives a bad colour to the terrain near it, so force it to only sample colours above.
		c = clamp(c, 0.0, 1.0);
		vec3 rayleigh;
		vec3 mie;
		calculateRayleighAndMie(V, L, Z, rayleigh, mie);
		vec4 apColour = vec4((rayleigh + mie) * PC.sunIntensity.xyz, 1.0);
		colour = texture(geometryColourBuffer, pos);
		colour = mix(colour, apColour, depth * depth);
	}
	else {
		//vec3 zenith = normalize(cameraPosition.xyz);
		vec3 V = normalize((PC.inverseViewProj * vec4(pos * 2.0 - 1.0, 1.0, 1.0)).xyz);
		vec3 rayleigh;
		vec3 mie;
		calculateRayleighAndMie(V, L, Z, rayleigh, mie);
		
		colour = vec4(rayleigh + mie, 1.0) * vec4(PC.sunIntensity.xyz, 1.0);
		colour = colour / (colour + vec4(1.0, 1.0, 1.0, 0.0));
		colour.rgb = pow(colour.rgb, vec3(1.0/2.2));
	}
}