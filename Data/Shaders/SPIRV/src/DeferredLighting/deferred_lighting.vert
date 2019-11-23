#version 450
#extension GL_GOOGLE_include_directive : enable
#define USE_MVP_BUFFER
#define USE_VERTEX_PUSH_CONSTANTS
#include "../common.glsl"

layout(constant_id = 0) const uint SC_MVP_OFFSET = 0;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormals;

layout(location = 0) flat out uint outInstanceIndex;
layout(location = 1) flat out vec4 outCameraPos;
layout(location = 2) flat out mat4 outShadowMatrix;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	gl_Position = mvps[SC_MVP_OFFSET + gl_InstanceIndex] * vec4(inPosition, 1.0);
	outInstanceIndex = gl_InstanceIndex;
	outCameraPos = PC.cameraPosition;
	outShadowMatrix = PC.shadowMatrix;
}
