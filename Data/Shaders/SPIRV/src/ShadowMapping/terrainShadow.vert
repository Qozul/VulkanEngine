#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout(push_constant) uniform PushConstants {
	uint mvpOffset;
	uint heightmapIdx;
};

layout(set = 0, binding = 0) readonly buffer StorageBuffer {
    mat4[] data;
} mvps;

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

const float maxHeight = 100.0;

void main() {
	vec3 pos = iPosition;
	pos.y -= texture(texSamplers[nonuniformEXT(heightmapIdx)], iTextureCoord).r * maxHeight;
	gl_Position = mvps.data[mvpOffset] * vec4(pos, 1.0);
}