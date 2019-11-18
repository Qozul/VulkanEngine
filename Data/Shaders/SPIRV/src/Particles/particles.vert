#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inScale;
layout(location = 2) in vec2 inTexOffset;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out float outScale;
layout(location = 2) out vec2 outTexOffset;
layout(location = 3) flat out int outInstanceIndex;
layout(location = 4) flat out vec4 outCameraPos;

layout(push_constant) uniform PushConstants {
	vec4 cameraPos;
} PC;

void main()
{
	outCameraPos = PC.cameraPos;
	outPosition = inPosition;
	outScale = inScale;
	outTexOffset = inTexOffset;
	outInstanceIndex = gl_InstanceIndex;
	gl_Position = vec4(inPosition, 1.0);
}
