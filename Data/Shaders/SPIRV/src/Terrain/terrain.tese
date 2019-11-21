#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"
#include "terrain_structs.glsl"

layout(constant_id = 0) const uint SC_MVP_OFFSET = 0;
layout(constant_id = 1) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 2) const uint SC_MATERIAL_OFFSET = 0;

layout(quads, equal_spacing, cw) in;

layout (location = 0) in vec2 iTexUV[];
layout (location = 1) flat in int instanceIndex[];
layout (location = 2) in vec4 shadowCoord[];
layout (location = 3) flat in uint shadowMapIdx[];
layout (location = 4) in vec3 inNormal[];


layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec3 normal;
layout (location = 3) flat out int outInstanceIndex;
layout (location = 4) out vec4 outShadowCoord;
layout (location = 5) out flat uint outShadowMapIdx;
layout (location = 6) out vec3 Fext;
layout (location = 7) out vec3 Lin;

layout(set = COMMON_SET, binding = COMMON_MVP_BINDING) readonly buffer UniformBufferObject {
    mat4 elementData[];
} ubo;

layout(set = GLOBAL_SET, binding = LIGHT_UBO_BINDING) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer MaterialData
{
	Params materials[];
};

layout(set = COMMON_SET, binding = COMMON_MATERIALS_BINDING) readonly buffer TexIndices
{
	TextureIndices textureIndices[];
};

const vec3 betaRay = vec3(6.55e-6, 1.73e-5, 2.30e-5);
const vec3 betaMie = vec3(2e-6);

// theta is the angle between the direction of the incident light and the direction of the scattered light
float rayleighPhase(float ctheta)
{
	return 0.8 * (1.4 + 0.5 * ctheta);
}

// g is in range [-1, 1]
float miePhase(float ctheta, float g)
{
	float g2 = g * g;
	float c2theta = ctheta * ctheta;
	return ((3.0 * (1.0 - g2)) / (2.0 * (2.0 + g2))) * ((1.0 + c2theta) / pow(1.0 + g2 - 2.0 * g * c2theta, 1.5));
}

void main(void)
{
	outInstanceIndex = instanceIndex[0];
	outShadowMapIdx = shadowMapIdx[0];
	Params material = materials[SC_PARAMS_OFFSET + instanceIndex[0]];
	TextureIndices texIndices = textureIndices[SC_MATERIAL_OFFSET + instanceIndex[0]];
	vec2 uv1 = mix(iTexUV[0], iTexUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(iTexUV[3], iTexUV[2], gl_TessCoord.x);
	texUV = mix(uv1, uv2, gl_TessCoord.y);

	vec4 shadowCoord1 = mix(shadowCoord[0], shadowCoord[1], gl_TessCoord.x);
	vec4 shadowCoord2 = mix(shadowCoord[3], shadowCoord[2], gl_TessCoord.x);
	outShadowCoord = mix(shadowCoord1, shadowCoord2, gl_TessCoord.y);
	
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	
	gl_Position = ubo.elementData[SC_MVP_OFFSET + instanceIndex[0]] * position;
	worldPos = (material.model * position).xyz;
	
	vec3 norm1 = mix(inNormal[0], inNormal[1], gl_TessCoord.x);
	vec3 norm2 = mix(inNormal[3], inNormal[2], gl_TessCoord.x);
	normal = mix(norm1, norm2, gl_TessCoord.y);
	normal = normalize(mat3(transpose(inverse(material.model))) * normal);
	
	Fext = exp(-(betaRay + betaMie) * distance(worldPos, cameraPosition.xyz));
	float ctheta = dot(normalize(worldPos - cameraPosition.xyz), normalize(lightPositions[0].xyz));
	Lin = ((rayleighPhase(ctheta) + miePhase(ctheta, 0.9)) / (betaRay + betaMie)) * (1.0 - Fext);
}
