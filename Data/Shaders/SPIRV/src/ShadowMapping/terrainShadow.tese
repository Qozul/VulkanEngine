#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(quads, equal_spacing, cw) in;

layout(location = 0) flat in uint mvpOffset[];
layout(location = 1) flat in uint heightmapIdx[];
layout(location = 2) in vec2 texUV[];

layout(set = 0, binding = 0) readonly buffer StorageBuffer {
    mat4[] data;
} mvps;

layout(set = 1, binding = 2) uniform sampler2D texSamplers[];

const float maxHeight = 100.0;

void main() {
	vec2 uv1 = mix(texUV[0], texUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(texUV[3], texUV[2], gl_TessCoord.x);
	vec2 uvs = mix(uv1, uv2, gl_TessCoord.y);
	
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	//position.y -= texture(texSamplers[nonuniformEXT(heightmapIdx[0])], uvs).r * maxHeight;
	gl_Position = mvps.data[mvpOffset[0]] * position;
}