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
layout(set = 1, binding = 1) uniform sampler2D transmittanceTexture;

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

const vec3 sunIntensity =  vec3(6.5e-7, 5.1e-7, 4.75e-7) * vec3(1e7);

void funkyAP(in float linearDepth)
{
	float inverseLinearDepth = linearDepth;
	inverseLinearDepth *= -1.0;
	inverseLinearDepth += 1.0;
	vec3 npwp = (PC.inverseViewProj * vec4(uv * 2.0 - 1.0, 1.0, 1.0)).xyz;
	vec3 Po = (PC.inverseViewProj * vec4(uv * 2.0 - 1.0, inverseLinearDepth * 2.0 - 1.0, 1.0)).xyz;
	npwp.y = Po.y;
	vec3 V = normalize(Po - npwp);
	vec3 Z = vec3(0.0, 1.0, 0.0);
	vec3 sDir = PC.sunDirection;
	sDir.y = max(0.0, sDir.y);
	vec3 L = normalize(sDir);
	float Cv = dot(V, Z);
	float Cs = dot(L, Z);
	// 1
	float height = 10.0f;
	vec4 IabTab = texture(scatteringTexture, vec3(heightToUh(height, PC.Hatm), 
		CvToUv(Cv, height, PC.planetRadius), CsToUs(Cs)));
	// 2
	height = clamp(Po.y, 0.0, PC.Hatm);
	vec4 IsbTsb = texture(scatteringTexture, vec3(heightToUh(height, PC.Hatm), 
		CvToUv(Cv, height, PC.planetRadius), CsToUs(Cs)));
	// 3
	vec4 Tas = texture(transmittanceTexture, vec2(heightToUh(height, PC.Hatm), CvToUv(Cv, height, PC.planetRadius)));
	vec4 IsbTab = Tas * IsbTsb;
	// 4
	vec3 IasTas = (IabTab - IsbTab).rgb;

	float ctheta = dot(V, L);
	IasTas *= rayleighPhase(ctheta);
	vec4 inscattering = clamp(vec4(IasTas, 1.0) * vec4(sunIntensity, 1.0), vec4(0.04), vec4(1.0));
	inscattering = inscattering / (inscattering + vec4(1.0, 1.0, 1.0, 0.0));
	inscattering.rgb = pow(inscattering.rgb, vec3(1.0/1.4));
	colour = mix(texture(geometryColourTexture, uv), inscattering, linearDepth * linearDepth);
}

void main()
{
	float depth = texture(geometryDepthTexture, uv).r;
	depth = clamp(linearizeDepth(depth), 0.0, 1.0);
	if (depth >= 1.0) {
		colour = texture(geometryColourTexture, uv);
	}
	else {
		funkyAP(depth);
	}
}
