#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 inPos;

void main() {

	fragColor = vec4(inPos, 1.0);
}
