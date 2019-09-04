#define INTEGRATION_STEPS 30
#define INVOCATION_SIZE 8

const float PI = 3.14159265358979323846;
const vec2 ZENITH_DIRECTION = vec2(1.0, 0.0);

//  48 bytes, 16-byte aligned
struct AtmosphereParameters {
	vec3 betaRay;
	float planetRadius;
	
	vec3 betaOzoneExt;
	float rayleighScaleHeight;
	
	float betaMie;
	float betaMieExt;
	float Hatm;
	float mieScaleHeight;
};

float UhToHeight(float Uh, float Hatm) {
	return max(Uh*Uh*Hatm, 1.0);
}

float heightToUh(float h, float Hatm) {
	return sqrt(h/Hatm);
}

float UvToCv(float Uv, float height, float planetRadius) {
	float Ch = -sqrt(height * (2.0 * planetRadius + height)) / (planetRadius + height);
	return Uv > 0.5 ? 
		Ch + pow(2.0 * Uv - 1.0, 5.0) * (1.0 - Ch) : 
		Ch - pow(2.0 * (0.5 - Uv), 5.0) * (1.0 + Ch);
}

float CvToUv(float Cv, float height, float planetRadius) {
	float Ch = -sqrt(height * (2.0 * planetRadius + height)) / (planetRadius + height);
	return Cv > Ch ?
		0.5 * pow((Cv - Ch) / (1.0 - Ch), 0.2) + 0.5 :
		0.5 - (0.5 * pow((Ch - Cv) / (Ch + 1.0), 0.2));
}

float UsToCs(float Us) {
	return clamp(tan((2.0 * Us - 1.0 + 0.26) * 0.75) / tan(1.26 * 0.75), -1.0, 1.0);
}

float CsToUs(float Cs) {
	return 0.5 * (atan(max(Cs, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1 - 0.26));
}

vec2 getRay(float Cv) {
    return normalize(vec2(Cv, sqrt(1.0 - Cv * Cv)));
}

vec3 getTransmittance(in float betaMieExt, in vec3 betaRayExt, 
	in float mieDensitySum, in float rayleighDensitySum)
{
	return exp(-(betaRayExt * rayleighDensitySum + betaMieExt * mieDensitySum ));
}

float getDensity(in float height, in float scaleHeight)
{
	return exp(-height/scaleHeight);
}

// Reference: https://github.com/Ralith/fuzzyblue
const float infinity = 1. / 0.;
float distanceToCircle(vec2 start, vec2 dir, float radius, bool nearest) {
    float c = dot(start, start) - (radius*radius);
    float b = dot(dir, start);
    float d = b*b - c;
    if (d < 0.0) {
		return infinity;
	}
    float t0 = -b - sqrt(d);
    float t1 = -b + sqrt(d);
    float ta = min(t0, t1);
    float tb = max(t0, t1);
    if (tb < 0.0) { 
		return infinity;
	}
    else if (nearest) { 
		return ta > 0.0 ? ta : tb;
	}
    else { 
		return tb;
	}
}

vec2 intersection(vec2 start, vec2 dir, float planetRadius, float Hatm) {
    float t = distanceToCircle(start, dir, planetRadius, true);
    if (isinf(t)) { t = distanceToCircle(start, dir, planetRadius + Hatm, false); }
    if (isinf(t)) { t = 0; }
    return start + t * dir;
}

// Assumes the centre of the planet is vec3(0.0)
float getHeight(in vec2 P, float planetRadius)
{
	return length(P) - planetRadius;
}

vec3 extractMieFromScattering(in vec4 scattering, in float betaMie, in vec3 betaRay)
{
	return scattering.rgb * (scattering.a / scattering.r) * (betaRay.r / betaMie) * (betaMie / betaRay);
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

vec3 getNearPlaneWorldPosition(in vec2 uv, in mat4 invViewProj)
{
	// Clip space is in range -1 to 1. z = -1.0 puts it on the near plane.
	vec4 clipCoords = vec4(uv * 2.0 - 1.0, -1.0, 1.0);
	// Use inverseViewProj to go from clip to world position
	vec4 pos = (invViewProj * clipCoords);
	// Perspective division
	pos /= pos.w;
	return pos.xyz;
}
