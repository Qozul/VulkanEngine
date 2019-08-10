#version 450
#extension GL_ARB_separate_shader_objects : enable

struct ElementData {
	mat4 model;
    mat4 mvp;
};

layout(location = 0) in vec3 iPosition;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    ElementData uElementData;
} ubo;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4(iPosition, 1.0);
	//gl_Position = ubo.uElementData.mvp * vec4(iPosition, 1.0);
}
