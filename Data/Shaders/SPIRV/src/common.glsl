#extension GL_EXT_nonuniform_qualifier : require
#define LIGHT_UBO_BINDING 0
#define ENVIORNMENT_SAMPLER_BINDING 1
#define CAMERA_INFO_BINDING 2
#define POST_PROCESS_BINDING 3
#define SAMPLER_ARRAY_BINDING 4
#define COMMON_MVP_BINDING 0
#define COMMON_PARAMS_BINDING 1
#define COMMON_MATERIALS_BINDING 2
#define COMMON_SET 0
#define GLOBAL_SET 1
#define MAX_LIGHTS 250

// ------------------- CONSTANTS ------------------
const mat4 BIAS_MATRIX = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0);

const float MIN_TESSELLATION_WEIGHT = 1.0;
const float MAX_TESSELLATION_WEIGHT = 64.0;

struct Light {
	vec3 position;
	float radius;
	vec3 colour;
	float attenuationFactor;
};

// ------------------- LAYOUTS ------------------

#ifdef USE_VERTEX_PUSH_CONSTANTS
layout(push_constant) uniform PushConstants {
	mat4 shadowMatrix;
	vec4 cameraPosition;
	vec3 mainLightPosition;
	uint shadowTextureIdx;
} PC;
#endif
#ifdef USE_MVP_BUFFER
layout(set = COMMON_SET, binding = COMMON_MVP_BINDING) readonly buffer ParameterData
{
	mat4[] mvps;
};
#endif

#ifndef OVERRIDE_TEX_SAMPLERS
layout(set = GLOBAL_SET, binding = SAMPLER_ARRAY_BINDING) uniform sampler2D texSamplers[];
#endif
#ifdef USE_LIGHTS_UBO
layout(set = GLOBAL_SET, binding = LIGHT_UBO_BINDING) uniform LightsData
{
	Light lights[MAX_LIGHTS];
};
#endif

// ----------------- FUNCTIONS ------------------

#ifndef OVERRIDE_TEX_SAMPLERS
float projectShadow(in vec4 shadowCoord, in vec2 off, in uint mapIdx)
{
	if (shadowCoord.s >= 0.0 && shadowCoord.s <= 1.0 && shadowCoord.t >= 0.0 && shadowCoord.t <= 1.0) { 
		return shadowCoord.z > -0.1 && shadowCoord.z < 1.0 && shadowCoord.w > 0.0 && 
			texture(texSamplers[nonuniformEXT(mapIdx)], shadowCoord.st + off).r < shadowCoord.z ? 0.1 : 1.0;
	}
	return 1.0;
}
#endif

void reinhardTonemap(inout vec4 colour) 
{
	colour = colour / (colour + vec4(1.0, 1.0, 1.0, 0.0));
	colour.rgb = pow(colour.rgb, vec3(1.0 / 2.2));
}

void calculatePhongShading(in vec3 worldPos, in vec3 lightPos, in vec3 camPos, in float radius, 
	in vec3 normal, in float shininess, out float lambert, out float specularFactor)
{
	vec3 incident = normalize(lightPos - worldPos);
	vec3 viewDir = normalize(camPos - worldPos);
	vec3 halfDir = normalize(incident + viewDir);
	
	float dist = length(lightPos - worldPos);
	float attenuation = 1.0 - clamp(dist / radius, 0.0, 1.0);
	float rFactor = max(0.0, dot(halfDir, normal));

	lambert = max(0.0, dot(incident, normal));
	specularFactor = pow(rFactor, shininess);
}

#ifndef OVERRIDE_TEX_SAMPLERS
vec3 getWorldPosition(uint depthIdx, vec2 uv, mat4 inverseViewProj)
{
	vec3 coords = vec3(uv * 2.0 - 1.0, texture(texSamplers[nonuniformEXT(depthIdx)], uv).r * 2.0 - 1.0);
	vec4 clip = inverseViewProj * vec4(coords, 1.0);
	return clip.xyz / clip.w;
}
#endif

float linearizeDepth(float depth, float near, float far)
{
  return (2.0 * near) / (far + near - depth * (far - near));
}

mat3 calculateTBN(in vec3 normal, in vec3 tangent)
{
	return mat3(tangent, cross(normal, tangent), normal);
}
