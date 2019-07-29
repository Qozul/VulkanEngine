#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout (location = 0) out vec2 texUV;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4(iPosition, 1.0);
	texUV = iTextureCoord;
}
