#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"
#include "terrain_structs.glsl"

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout(location = 0) out vec4 fragColor;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec3 normal;
layout (location = 3) flat in int instanceIndex;
layout (location = 4) in vec4 shadowCoord;
layout (location = 5) flat in uint shadowMapIdx;

layout(set = GLOBAL_SET, binding = LIGHT_UBO_BINDING) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer ParamsData
{
	Params params[];
};
layout(set = COMMON_SET, binding = COMMON_MATERIALS_BINDING) readonly buffer TexIndices
{
	TextureIndices textureIndices[];
};

//See reference: https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/shadowmapping/scene.frag
float texturePrj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture(texSamplers[nonuniformEXT(shadowMapIdx)], shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.1;
		}
	}
	return shadow;
}

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
	vec3 diffuse = max(texColour.rgb * param.diffuseColour.xyz * lambert, ambient);
	vec3 specular = param.specularColour.xyz * sFactor * 0.05;
	
	float shadow = texturePrj(shadowCoord / shadowCoord.w, vec2(0.0));
	fragColor = vec4((diffuse + specular) * shadow, min(texColour.a, param.diffuseColour.w));
	fragColor = fragColor / (fragColor + vec4(1.0, 1.0, 1.0, 0.0));
	fragColor.rgb = pow(fragColor.rgb, vec3(1.0/2.2));
}
