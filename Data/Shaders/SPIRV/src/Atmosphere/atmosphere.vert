#version 450
#extension GL_ARB_separate_shader_objects : enable

// Unit square in 2d xy
layout(location = 0) in vec3 iPosition;
layout(location = 0) out vec2 pos;

out gl_PerVertex {
	vec4 gl_Position;
};
/*
layout(push_constant) uniform ScreenExtent {
	mat4 inverseViewProj;
} PC;*/

const vec2 extent = vec2(800.0, 600.0);

void main() {
	pos =  iPosition.xy * extent;
	gl_Position = vec4(pos, 1.0, 1.0);
	//viewDirection = vec3(0.0, 1.0, 0.0);//normalize((PC.inverseViewProj * pos).xyz);
}
