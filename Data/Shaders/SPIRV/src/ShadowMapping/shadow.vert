#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout(push_constant) uniform PushConstants {
	uint mvpOffset;
};

layout(set = 0, binding = 0) readonly buffer StorageBuffer {
    mat4[] data;
} mvps;

void main() {
	gl_Position = mvps.data[mvpOffset + gl_InstanceIndex] * vec4(iPosition, 1.0);
}