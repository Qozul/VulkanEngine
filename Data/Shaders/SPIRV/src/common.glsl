#extension GL_EXT_nonuniform_qualifier : require
#define LIGHT_UBO_BINDING 0
#define ENVIORNMENT_SAMPLER_BINDING 1
#define SAMPLER_ARRAY_BINDING 2
#define COMMON_MVP_BINDING 0
#define COMMON_PARAMS_BINDING 1
#define COMMON_MATERIALS_BINDING 2
#define COMMON_SET 0
#define GLOBAL_SET 1

#ifndef OVERRIDE_TEX_SAMPLERS
layout(set = GLOBAL_SET, binding = SAMPLER_ARRAY_BINDING) uniform sampler2D texSamplers[];
#endif

const mat4 BIAS_MATRIX = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0);

const float MIN_TESSELLATION_WEIGHT = 1.0;
const float MAX_TESSELLATION_WEIGHT = 64.0;

struct CommonVertexPushConstants {
	mat4 shadowMatrix;
	vec4 cameraPosition;
	vec3 mainLightPosition;
	uint shadowTextureIdx;
};

struct Light {
	vec3 position;
	float radius;
	vec3 colour;
	float attenuationFactor;
	vec4 direction;
};

#ifndef OVERRIDE_TEX_SAMPLERS
//See reference: https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/shadowmapping/scene.frag
float projectShadow(in vec4 shadowCoord, in vec2 off, in uint mapIdx)
{
	float shadow = 1.0;
	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
	{
		float dist = texture(texSamplers[nonuniformEXT(mapIdx)], shadowCoord.st + off).r;
		if (shadowCoord.w > 0.0 && dist < shadowCoord.z)
		{
			shadow = 0.1;
		}
	}
	return shadow;
}
#endif

void reinhardTonemap(inout vec4 colour) 
{
	colour = colour / (colour + vec4(1.0, 1.0, 1.0, 0.0));
	colour.rgb = pow(colour.rgb, vec3(1.0 / 2.2));
}

void calculatePhongShading(in vec3 worldPos, in vec3 lightPos, in vec3 camPos, in vec3 normal, in float shininess, out float lambert, out float specularFactor)
{
	vec3 incident = normalize(lightPos - worldPos);
	vec3 viewDir = normalize(camPos - worldPos);
	vec3 halfDir = normalize(incident + viewDir);
	float dist = length(lightPos - worldPos);
	float rFactor = max(0.0, dot(halfDir, normal));

	lambert = max(0.0, dot(incident, normal));
	specularFactor = pow(rFactor, shininess);
}
