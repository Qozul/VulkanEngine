#version 450
#extension GL_GOOGLE_include_directive : enable
#define USE_MVP_BUFFER
#define USE_VERTEX_PUSH_CONSTANTS
#include "../common.glsl"

struct Params {
    mat4 model;
	vec4 diffuseColour;
	vec4 specularColour;
};

layout(constant_id = 0) const uint SC_MVP_OFFSET = 0;
layout(constant_id = 1) const uint SC_PARAMS_OFFSET = 0;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outWorldPos;
layout(location = 3) flat out int outInstanceIndex;
layout(location = 4) out vec4 outShadowCoord;
layout(location = 5) flat out uint outShadowMapIdx;
layout(location = 6) flat out vec3 outCamPos;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer ParamsData
{
	Params params[];
};

void main() {
	outInstanceIndex = gl_InstanceIndex;
	gl_Position = mvps[SC_MVP_OFFSET + gl_InstanceIndex] * vec4(inPosition, 1.0);
	outUV = inTextureCoord;
	outWorldPos = (params[SC_PARAMS_OFFSET + gl_InstanceIndex].model * vec4(inPosition, 1.0)).xyz;
	outNormal = mat3(transpose(inverse(params[SC_PARAMS_OFFSET + gl_InstanceIndex].model))) * inNormal;

	outShadowCoord = (BIAS_MATRIX * PC.shadowMatrix * params[SC_PARAMS_OFFSET + gl_InstanceIndex].model) * vec4(inPosition, 1.0);
	outShadowMapIdx = PC.shadowTextureIdx;
	outCamPos = PC.cameraPosition.xyz;
}
