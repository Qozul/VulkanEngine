// Reference: https://github.com/Ralith/fuzzyblue for distanceToCircle, intersection,
// and adjustments to the parametrization values.
// Additionally, this scattering is based on the paper http://publications.lib.chalmers.se/records/fulltext/203057/203057.pdf by
// Gustav Boadre and Edvard Sandberg.

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
	return 0.5 * (atan(max(Cs, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26));
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

const float inf = 1. / 0.;
float distanceToCircle(vec2 start, vec2 dir, float radius, bool nearest) {
    float c = dot(start, start) - (radius*radius);
    float b = dot(dir, start);
    float d = b*b - c;
    if (d < 0.0) return inf;
	
    float t0 = -b - sqrt(d);
    float t1 = -b + sqrt(d);
    float ta = min(t0, t1);
    float tb = max(t0, t1);
	return tb < 0.0 ? inf : nearest && ta > 0.0 ? ta : tb;
}

vec2 intersection(vec2 start, vec2 dir, float planetRadius, float Hatm) {
    float t = distanceToCircle(start, dir, planetRadius, true);
    t = isinf(t) ? distanceToCircle(start, dir, planetRadius + Hatm, false) : t;
    t = isinf(t) ? 0 : t;
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
