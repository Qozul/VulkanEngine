#version 450
#extension GL_EXT_nonuniform_qualifier : require

struct ElementData {
	mat4 model;
    mat4 mvp;
};
struct TextureIndices {
	uint heightmapIdx;
	uint normalmapIdx;
	uint diffuseIdx;
};
layout(quads, equal_spacing, cw) in;

layout (location = 0) in vec2 iTexUV[];
layout (location = 1) flat in int instanceIndex[];

layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec3 normal;
layout (location = 3) flat out int outInstanceIndex;

layout(set = 0, binding = 0) readonly buffer UniformBufferObject {
    ElementData uElementData;
} ubo;

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};
layout(set = 0, binding = 3) readonly buffer TexIndices
{
	TextureIndices textureIndices;
};

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

const float maxHeight = 100.0;

void main(void)
{
	outInstanceIndex = instanceIndex[0];
	TextureIndices texIndices = textureIndices;
	vec2 uv1 = mix(iTexUV[0], iTexUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(iTexUV[3], iTexUV[2], gl_TessCoord.x);
	texUV = mix(uv1, uv2, gl_TessCoord.y);
	
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	position.y -= texture(texSamplers[nonuniformEXT(texIndices.heightmapIdx)], texUV).r * maxHeight;
	
	gl_Position = ubo.uElementData.mvp * position;
	worldPos = (ubo.uElementData.model * position).xyz;
	normal = texture(texSamplers[nonuniformEXT(texIndices.normalmapIdx)], texUV).rgb;
	float tmp = normal.r;
	normal.r = normal.g;
	normal.g = normal.b;
	normal.b = tmp;
}
