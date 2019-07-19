#version 460 core

struct InstanceData
{
	mat4 model;
	mat4 mvp;
};

layout(std430, binding = 0) buffer InstanceDataBuffer
{
    InstanceData[] uInstanceData;
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
	int instanceID = gl_BaseInstance + gl_InstanceID;
	gl_Position	= uInstanceData[instanceID].mvp * vec4(iPosition, 1.0);
	OUT.colour = vec4(0.6, 0.7, 0.8, 1.0);
	OUT.worldPos = (uInstanceData[instanceID].model * vec4(iPosition, 1.0)).xyz;
	OUT.normal = mat3(transpose(inverse(uInstanceData[instanceID].model))) * iNormal;
}
