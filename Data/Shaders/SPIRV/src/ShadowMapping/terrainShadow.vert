#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout(location = 0) flat out uint mvpOffset;
layout(location = 1) flat out uint heightmapIdx;
layout(location = 2) out vec2 texUV;

layout(push_constant) uniform PushConstants {
	uint mvpOffset;
	uint heightmapIdx;
}PC;

void main() {
	mvpOffset = PC.mvpOffset;
	heightmapIdx = PC.heightmapIdx;
	texUV = iTextureCoord;
	gl_Position = vec4(iPosition, 1.0);
}