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
layout (location = 4) in vec2 inNormalizedUvs[];
layout (location = 5) flat in float maxHeight[];

layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec3 normal;
layout (location = 3) flat out int outInstanceIndex;
layout (location = 4) out vec4 outShadowCoord;
layout (location = 5) out flat uint outShadowMapIdx;

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
	
	vec2 normalUvs1 = mix(inNormalizedUvs[0], inNormalizedUvs[1], gl_TessCoord.x);
	vec2 normalUvs2 = mix(inNormalizedUvs[3], inNormalizedUvs[2], gl_TessCoord.x);
	vec2 normalizedUvs = mix(normalUvs1, normalUvs2, gl_TessCoord.y);
	
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	position.y -= texture(texSamplers[nonuniformEXT(texIndices.heightmapIdx)], normalizedUvs).r * maxHeight[0];
	
	gl_Position = ubo.elementData[SC_MVP_OFFSET + instanceIndex[0]] * position;
	worldPos = (material.model * position).xyz;
	normal = texture(texSamplers[nonuniformEXT(texIndices.normalmapIdx)], normalizedUvs).rgb;
	float tmp = normal.r;
	normal.r = normal.g;
	normal.g = normal.b;
	normal.b = tmp;
}
