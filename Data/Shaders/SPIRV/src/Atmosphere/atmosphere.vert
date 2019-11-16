#version 450
#extension GL_ARB_separate_shader_objects : enable

// Unit square in 2d xy
layout (location = 0) in vec3 iPosition;
layout (location = 0) out vec2 pos;
layout (location = 1) flat out vec4 cameraPos;

layout (push_constant) uniform PushConstants {
	vec4 cameraPos;
} PC;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	pos = iPosition.xy;
	cameraPos = PC.cameraPos;
	gl_Position = vec4(pos * 2.0 - 1.0, 1.0, 1.0);
}
