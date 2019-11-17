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

layout(location = 0) out vec4 fragColor;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 worldPos;
layout (location = 3) flat in int instanceIndex;

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

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

void main() {
	Material mat = materials[SC_PARAMS_OFFSET + instanceIndex];
	vec3 incident = normalize ( lightPositions[0].xyz - worldPos );
	vec3 viewDir = normalize ( cameraPosition.xyz - worldPos );
	vec3 halfDir = normalize ( incident + viewDir );
	float dist = length(lightPositions[0].xyz - worldPos);
	//float atten = 1.0 - clamp ( dist / lightRadius , 0.0 , 1.0);
	float lambert = max(0.0, dot(incident, normal));
	float rFactor = max(0.0, dot(halfDir, normal));
	float sFactor = pow(rFactor , mat.specularColour.w);
	
	DescriptorIndexData descriptorIndices = diData[SC_MATERIAL_OFFSET + instanceIndex];
	vec4 texColour = texture(texSamplers[nonuniformEXT(descriptorIndices.diffuseIdx)], texUV);
	vec4 texColour2 = texture(texSamplers[nonuniformEXT(descriptorIndices.normalMapIdx)], texUV);
	
	vec3 ambient = texColour.rgb * ambientColour.xyz;
	vec3 diffuse = texColour.rgb * mat.diffuseColour.xyz * lambert;
	vec3 specular = mat.specularColour.xyz * sFactor * 0.05;
	fragColor = vec4(ambient + diffuse + specular, min(texColour.a, mat.diffuseColour.w));
}
