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
	vec2 texUV;
	vec3 worldPos;
	vec3 normal;
	flat int instanceID;
} OUT;

void main(void)
{
	OUT.instanceID = gl_BaseInstance + gl_InstanceID;
	gl_Position	= uInstanceData[OUT.instanceID].mvp * vec4(iPosition, 1.0);
	OUT.texUV = iTexUV;
	OUT.worldPos = (uInstanceData[OUT.instanceID].model * vec4(iPosition, 1.0)).xyz;
	OUT.normal = mat3(transpose(inverse(uInstanceData[OUT.instanceID].model))) * iNormal;
}
