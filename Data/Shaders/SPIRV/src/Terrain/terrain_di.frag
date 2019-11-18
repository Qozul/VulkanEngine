#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

struct Params {
	mat4 model;
	vec4 diffuseColour;
	vec4 specularColour;
};

struct TextureIndices {
	uint heightmapIdx;
	uint normalmapIdx;
	uint diffuseIdx;
};

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout(location = 0) out vec4 fragColor;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec3 normal;
layout (location = 3) flat in int instanceIndex;

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = 0, binding = 1) readonly buffer ParamsData
{
	Params params[];
};
layout(set = 0, binding = 2) readonly buffer TexIndices
{
	TextureIndices textureIndices[];
};

void main() {
	Params param = params[SC_PARAMS_OFFSET + instanceIndex];
	TextureIndices texIndices = textureIndices[SC_MATERIAL_OFFSET + instanceIndex];
	vec3 incident = normalize(lightPositions[0].xyz - worldPos);
	vec3 viewDir = normalize(cameraPosition.xyz - worldPos);
	vec3 halfDir = normalize(incident + viewDir);
	float dist = length(lightPositions[0].xyz - worldPos);
	
	float lambert = max(0.0, dot(incident, normal));
	float rFactor = max(0.0, dot(halfDir, normal));
	float sFactor = pow(rFactor , param.specularColour.w);
	
	vec4 texColour = texture(texSamplers[nonuniformEXT(texIndices.diffuseIdx)], texUV);
	
	vec3 ambient = texColour.rgb * ambientColour.xyz;
	vec3 diffuse = texColour.rgb * param.diffuseColour.xyz * lambert;
	vec3 specular = param.specularColour.xyz * sFactor * 0.05;
	fragColor = vec4(ambient + diffuse + specular, min(texColour.a, param.diffuseColour.w));
}
