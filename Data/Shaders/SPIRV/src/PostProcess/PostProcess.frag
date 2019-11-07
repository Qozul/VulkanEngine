#version 450
#extension GL_GOOGLE_include_directive : enable
#include "../Atmosphere/alt_functions.glsl"

layout (constant_id = 0) const float SC_NEAR_Z = 0.1;
layout (constant_id = 1) const float SC_FAR_Z = 1000.0;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

layout(push_constant) uniform PushConstants {
	mat4 inverseViewProj;
	vec3 camPos;
	float Hatm;
	vec3 sunDirection;
	float planetRadius;
	vec3 betaRay;
	float betaMie;
} PC;

layout(set = 0, binding = 0) uniform sampler2D geometryColourTexture;
layout(set = 0, binding = 1) uniform sampler2D geometryDepthTexture;
layout(set = 1, binding = 0) uniform sampler3D scatteringTexture;

float linearizeDepth(float depth)
{
  return (2.0 * SC_NEAR_Z) / (SC_FAR_Z + SC_NEAR_Z - depth * (SC_FAR_Z - SC_NEAR_Z));
}

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

void main()
{
	vec4 geometryColour = texture(geometryColourTexture, uv);
	float depth = texture(geometryDepthTexture, uv).r;
	depth = clamp(linearizeDepth(depth), 0.0, 1.0);
	if (depth >= 1.0) {
		colour = geometryColour;
	}
	else {
		vec3 V = normalize((PC.inverseViewProj * vec4(uv * 2.0 - 1.0, 1.0, 1.0)).xyz);
		vec3 Pb = PC.camPos + V * depth * SC_FAR_Z;
		V = normalize(vec3(V.x, Pb.y, V.z));
		vec3 Z = vec3(0.0, 1.0, 0.0);
		float height = clamp(Pb.y, 1.0, PC.Hatm);
		float Cv = dot(V, Z);
		float Cs = dot(normalize(PC.sunDirection), Z);
		vec4 scattering = texture(scatteringTexture, vec3(heightToUh(height, PC.Hatm), 
			CvToUv(Cv, height, PC.planetRadius), CsToUs(Cs)));
		vec3 rayleigh = scattering.rgb;
		vec3 mie = extractMieFromScattering(scattering, PC.betaMie, PC.betaRay);
		float ctheta = dot(V, normalize(PC.sunDirection));
		rayleigh *= rayleighPhase(ctheta);
		mie *= miePhase(ctheta, PC.Hatm);
		vec3 fogColour = vec3(rayleigh + mie) * vec3(6.5e-7, 5.1e-7, 4.75e-7) * vec3(1e7);
		colour = mix(geometryColour, vec4(fogColour, 1.0), 1.0);
		colour = colour / (colour + vec4(1.0, 1.0, 1.0, 0.0));
		colour.rgb = pow(colour.rgb, vec3(1.0/2.2));
	}
}
