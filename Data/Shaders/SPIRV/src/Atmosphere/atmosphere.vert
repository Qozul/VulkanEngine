#version 450
#extension GL_ARB_separate_shader_objects : enable

// Unit square in 2d xy
layout(location = 0) in vec3 iPosition;
layout(location = 0) out vec2 pos;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	pos = iPosition.xy;
	gl_Position = vec4(pos * 2.0 - 1.0, 1.0, 1.0);
}
