#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout(location = 0) flat out uint mvpOffset;

layout(push_constant) uniform PushConstants {
	uint mvpOffset;
}PC;

void main() {
	mvpOffset = PC.mvpOffset;
	gl_Position = vec4(iPosition, 1.0);
}