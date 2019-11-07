#version 450
#extension GL_GOOGLE_include_directive : enable
#include "../Atmosphere/alt_functions.glsl"
#define INTEGRATION_STEPS_TRANSMITTANCE 30

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
	return ((3.0 * (1.0 - g2)) / (2.0 * (2.0 + g2))) * ((1.0 + ctheta) / pow(1.0 + g2 - 2.0 * g * ctheta, 1.5));
}

const float mieScaleHeight = 1200.0;
const float rayleighScaleHeight = 8000.0;
const vec3 betaOzoneExt = vec3(5.09, 7.635, 0.2545);

vec3 transmittanceToObject(in vec3 Pa, in vec3 Pb, in vec3 v)
{
	float mieDensitySum = 0.0;
	float rayleighDensitySum = 0.0;
	float ozoneDensitySum = 0.0;
	float previousMieDensity = 0.0;
	float previousRayleighDensity = 0.0;
	float previousOzoneDensity = 0.0;
	// Integrate along viewing ray from corresponding height
	float stepSize = distance(Pa, Pb) / float(INTEGRATION_STEPS_TRANSMITTANCE);
	// For each integration step, calculate the particle density of Mie and Rayleigh
	for (int s = 0; s < INTEGRATION_STEPS_TRANSMITTANCE; ++s) {
		vec3 P = Pa + stepSize * s * v; // Integration point
		float h = clamp(P.y, 0.0, PC.Hatm);
		float mieDensity = getDensity(h, mieScaleHeight);
		float rayleighDensity = getDensity(h, rayleighScaleHeight);
		float ozoneDensity = 6e-7 * rayleighDensity;
		mieDensitySum += (mieDensity + previousMieDensity) / 2.0 * stepSize;
		rayleighDensitySum += (rayleighDensity + previousMieDensity) / 2.0 * stepSize;
		ozoneDensitySum += (ozoneDensity + previousOzoneDensity) / 2.0 * stepSize;
		previousMieDensity = mieDensity;
		previousRayleighDensity = rayleighDensity;
		previousOzoneDensity = ozoneDensity;
	}
	return exp(-(PC.betaRay * rayleighDensitySum + 
		(PC.betaMie / 0.9) * mieDensitySum + betaOzoneExt * ozoneDensitySum ));
}
const vec3 sunIntensity =  vec3(6.5e-7, 5.1e-7, 4.75e-7) * vec3(1e7);

void funkyAP(in float linearDepth)
{
	vec3 npwp = (PC.inverseViewProj * vec4(uv * 2.0 - 1.0, 1.0, 1.0)).xyz;
	vec3 V = normalize(npwp);
	vec3 Z = vec3(0.0, 1.0, 0.0);
	vec3 L = normalize(PC.sunDirection);
	vec3 Pc = PC.camPos;
	Pc.y = 0.0;
	vec3 Po = PC.camPos + V * linearDepth * ((SC_FAR_Z - SC_NEAR_Z) + SC_NEAR_Z);
	V = normalize(Po - Pc);
	float Cv = dot(V, Z);
	float Cs = dot(L, Z);
	// 1
	float height = Pc.y;
	vec4 IabTab = texture(scatteringTexture, vec3(heightToUh(height, PC.Hatm), 
		CvToUv(Cv, height, PC.planetRadius), CsToUs(Cs)));
	// 2
	height = clamp(Po.y, 0.0, PC.Hatm);
	vec4 IsbTsb = texture(scatteringTexture, vec3(heightToUh(height, PC.Hatm), 
		CvToUv(Cv, height, PC.planetRadius), CsToUs(Cs)));
	// 3
	vec4 Tas = vec4(transmittanceToObject(Pc, Po, V), 1.0);
	vec4 IsbTab = Tas * IsbTsb;
	// 4
	vec4 IasTas = IabTab - IsbTab;
	
	vec3 rayleigh = IasTas.rgb;
	vec3 mie = extractMieFromScattering(IasTas, PC.betaMie, PC.betaRay);
	float ctheta = dot(V, L);
	rayleigh *= rayleighPhase(ctheta);
	mie *= miePhase(ctheta, 0.76);
	vec4 inscattering = vec4(vec3(rayleigh * 20.0 + mie * 10.0), 1.0) * vec4(sunIntensity, 1.0);
	colour = texture(geometryColourTexture, uv) * Tas + inscattering;
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

/*
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
	}*/
}
