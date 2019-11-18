#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec2 pos;
layout (location = 1) flat out vec4 cameraPos;

layout (push_constant) uniform PushConstants {
	vec4 cameraPos;
} PC;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	cameraPos = PC.cameraPos;
	pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(pos * 2.0f - 1.0f, 0.0f, 1.0f);
}
