#version 400 core


layout(location = 0) in vec3 iPosition;
layout(location = 2) in vec2 iTexUV;

out Vertex {
	vec2 texUV;
} OUT;

void main(void) {
	gl_Position = vec4(iPosition, 1.0);
	OUT.texUV = iTexUV;
}
