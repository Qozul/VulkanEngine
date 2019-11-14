#version 450
#extension GL_EXT_nonuniform_qualifier : require
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
	uint colourIdx;
	uint depthIdx;
} PC;

layout(set = 0, binding = 1) uniform sampler2D texSamplers[];

float linearizeDepth(float depth)
{
  return (2.0 * SC_NEAR_Z) / (SC_FAR_Z + SC_NEAR_Z - depth * (SC_FAR_Z - SC_NEAR_Z));
}
const vec4 aerialPerspectiveColour = vec4(0.74, 0.84, 0.92, 1.0);

void main()
{
	float depth = texture(texSamplers[nonuniformEXT(PC.depthIdx)], uv).r;
	depth = clamp(linearizeDepth(depth), 0.0, 1.0);
	if (depth >= 1.0) {
		colour = texture(texSamplers[nonuniformEXT(PC.colourIdx)], uv);
	}
	else {
		colour = mix(texture(texSamplers[nonuniformEXT(PC.colourIdx)], uv), aerialPerspectiveColour, depth * depth);
	}
}
