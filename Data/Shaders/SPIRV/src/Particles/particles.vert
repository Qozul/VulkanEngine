#version 450
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inScale;
layout(location = 2) in vec2 inTexOffset;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out float outScale;
layout(location = 2) out vec2 outTexOffset;
layout(location = 3) flat out int outInstanceIndex;
layout(location = 4) flat out vec4 outCameraPos;

layout(push_constant) uniform PushConstants {
	mat4 shadowMatrix;
	vec4 cameraPosition;
	vec3 mainLightPosition;
	uint shadowTextureIdx;
} PC;

void main()
{
	outCameraPos = PC.cameraPosition;
	outPosition = inPosition;
	outScale = inScale;
	outTexOffset = inTexOffset;
	outInstanceIndex = gl_InstanceIndex;
	gl_Position = vec4(inPosition, 1.0);
}
