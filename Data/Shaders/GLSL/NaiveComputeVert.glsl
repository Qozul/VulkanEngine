#version 440 core

struct ElementData {
	mat4 model;
	mat4 mvp;
};

layout(std430, binding = 0) buffer IN0
{
    ElementData elementData;
};

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
	gl_Position	= elementData.mvp * vec4(iPosition, 1.0);
	OUT.colour = vec4(iNormal, 1.0);
	OUT.worldPos = (elementData.model * vec4(iPosition, 1.0)).xyz;
	OUT.normal = mat3(transpose(inverse(elementData.model))) * iNormal;
}
