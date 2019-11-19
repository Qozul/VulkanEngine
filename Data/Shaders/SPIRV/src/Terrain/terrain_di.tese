#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
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

layout(set = 0, binding = 0) readonly buffer UniformBufferObject {
    mat4 elementData[];
} ubo;

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = 0, binding = 1) readonly buffer MaterialData
{
	Params materials[];
};

layout(set = 0, binding = 2) readonly buffer TexIndices
{
	TextureIndices textureIndices[];
};

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

const float maxHeight = 100.0;

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
	//position.y -= texture(texSamplers[nonuniformEXT(texIndices.heightmapIdx)], texUV).r * maxHeight;

	vec3 norm1 = mix(inNormal[0], inNormal[1], gl_TessCoord.x);
	vec3 norm2 = mix(inNormal[3], inNormal[2], gl_TessCoord.x);
	normal = mix(norm1, norm2, gl_TessCoord.y);
	
	gl_Position = ubo.elementData[SC_MVP_OFFSET + instanceIndex[0]] * position;
	worldPos = (material.model * position).xyz;
	//normal = texture(texSamplers[nonuniformEXT(texIndices.normalmapIdx)], texUV).rgb;
	float tmp = normal.r;
	normal.r = normal.g;
	normal.g = normal.b;
	normal.b = tmp;
}
