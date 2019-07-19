#version 440 core

uniform mat4 uMVP;
uniform mat4 uModelMat;

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexUV;
layout(location = 2) in vec3 iNormal;

out Vertex
{
	vec4 colour;
	vec3 worldPos;
	vec3 normal;
} OUT;

void main(void)
{
	gl_Position	= uMVP * vec4(iPosition, 1.0);
	OUT.colour = vec4(0.6, 0.7, 0.8, 1.0);
	OUT.worldPos = (uModelMat * vec4(iPosition, 1.0)).xyz;
	OUT.normal = mat3(transpose(inverse(uModelMat))) * iNormal;
}
