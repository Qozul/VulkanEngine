#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inScale;
layout(location = 2) in vec2 inTexOffset;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out float outScale;
layout(location = 2) out vec2 outTexOffset;

void main()
{
	outPosition = inPosition;
	outScale = inScale;
	outTexOffset = inTexOffset;
}
