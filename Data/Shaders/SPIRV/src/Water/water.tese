#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

struct Params {
	mat4 model;
	vec4 baseColour; // .w = displacementOffset
	vec4 tipColour; // .w = maxHeight
};

struct Material {
	uint dudvIdx;
	uint displacementmapIdx;
	uint normalmapIdx;
};

layout(constant_id = 0) const uint SC_MVP_OFFSET = 0;
layout(constant_id = 1) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 2) const uint SC_MATERIAL_OFFSET = 0;

layout(quads, equal_spacing, cw) in;

layout (location = 0) in vec2 iTexUV[];
layout (location = 1) flat in int instanceIndex[];
layout (location = 2) flat in uint shadowMapIdx[];
layout (location = 3) flat in mat4 shadowMat[];

layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec3 normal;
layout (location = 3) flat out int outInstanceIndex;
layout (location = 4) out vec4 outShadowCoord;
layout (location = 5) flat out uint outShadowMapIdx;
layout (location = 6) out float height;
layout (location = 7) flat out uint disp;

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
	Params params[];
};

layout(set = 0, binding = 2) readonly buffer TexIndices
{
	Material textureIndices[];
};

layout(set = 1, binding = 2) uniform sampler2D texSamplers[];

void main(void)
{
	outInstanceIndex = instanceIndex[0];
	outShadowMapIdx = shadowMapIdx[0];
	Params param = params[SC_PARAMS_OFFSET + instanceIndex[0]];
	Material texIndices = textureIndices[SC_MATERIAL_OFFSET + instanceIndex[0]];
	vec2 uv1 = mix(iTexUV[0], iTexUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(iTexUV[3], iTexUV[2], gl_TessCoord.x);
	texUV = mix(uv1, uv2, gl_TessCoord.y) + param.baseColour.w;
	
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	height = texture(texSamplers[nonuniformEXT(texIndices.displacementmapIdx)], texUV).r;
	position.y -= height * param.tipColour.w;

	outShadowCoord = (BIAS_MATRIX * shadowMat[0] * param.model) * position;
	
	gl_Position = ubo.elementData[SC_MVP_OFFSET + instanceIndex[0]] * position;
	worldPos = (param.model * position).xyz;
	normal = texture(texSamplers[nonuniformEXT(texIndices.normalmapIdx)], texUV).rgb;
	float tmp = normal.r;
	normal.r = normal.g;
	normal.g = normal.b;
	normal.b = tmp;
	normal = normalize(mat3(transpose(inverse(param.model))) * normal);
	
	disp = texIndices.dudvIdx;
}
