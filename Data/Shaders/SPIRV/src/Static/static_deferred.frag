#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

struct Material {
    mat4 model;
	vec4 diffuseColour;
	vec4 specularColour;
};

struct DescriptorIndexData {
	uint diffuseIdx;
	uint normalMapIdx;
};

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout (location = 0) out vec4 position;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 albedo;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 worldPos;
layout (location = 3) flat in int instanceIndex;
layout (location = 4) in vec4 shadowCoord;
layout (location = 5) flat in uint shadowMapIdx;

layout(set = 1, binding = 2) uniform sampler2D texSamplers[];

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = 0, binding = 1) readonly buffer MaterialData
{
	Material materials[];
};

layout(set = 0, binding = 2) readonly buffer DescriptorIndexBuffer
{
	DescriptorIndexData diData[];
};

void main() 
{
	Material mat = materials[SC_PARAMS_OFFSET + instanceIndex];
	position = vec4(worldPos, 1.0);
	outNormal = vec4(normal, mat.specularColour.w);
	DescriptorIndexData descriptorIndices = diData[SC_MATERIAL_OFFSET + instanceIndex];
	albedo = texture(texSamplers[nonuniformEXT(descriptorIndices.diffuseIdx)], texUV);
}
