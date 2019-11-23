#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#define USE_LIGHTS_UBO
#include "../common.glsl"

struct Params {
	mat4 model;
	vec4 baseColour; // .w = displacementOffset
	vec4 tipColour; // .w = maxHeight
};

struct Material {
	uint displacementmapIdx;
	uint normalmapIdx;
};

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 inWorldPos;
layout (location = 2) in vec3 inNormal;
layout (location = 3) flat in int instanceIndex;
layout (location = 4) in vec4 shadowCoord;
layout (location = 5) flat in uint shadowMapIdx;
layout (location = 6) in float height;
layout (location = 7) flat in uint disp;
layout (location = 8) flat in vec3 outCamPos;

layout(set = 1,  binding = 1) uniform samplerCube Cubemap;

layout(set = 0, binding = 1) readonly buffer ParamsData
{
	Params params[];
};

void main() {
	Params param = params[SC_PARAMS_OFFSET + instanceIndex];
	vec3 distortion = texture(texSamplers[nonuniformEXT(disp)], texUV).rgb * 0.15;
	vec3 viewDir = normalize(outCamPos - inWorldPos);
	
	vec3 R = reflect(viewDir, inNormal);
	R += distortion;
	vec4 environmentCol = texture(Cubemap, R);
	
	vec3 albedoColour = mix(param.baseColour.xyz, param.tipColour.xyz, pow(height, 2.0));
	
	outPosition = vec4(inWorldPos, 1.0);
	outNormal = vec4(inNormal * 0.5 + 0.5, 4.0);
	
	outAlbedo = vec4(mix(albedoColour, environmentCol.rgb, 0.75), 1.0);
}
