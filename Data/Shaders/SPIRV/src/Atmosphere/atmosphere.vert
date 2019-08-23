#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 iPosition;
//layout(location = 1) in vec2 uvcoords;
layout(location = 0) out vec2 oUvcoords;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 model;
    mat4 mvp;
} ubo;

void main() {
	gl_Position = vec4(iPosition, 1.0);
	//oUvcoords = uvcoords;
	oUvcoords = vec2(0.0);
}
