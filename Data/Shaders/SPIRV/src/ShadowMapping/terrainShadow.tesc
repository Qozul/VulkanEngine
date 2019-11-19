#version 450

#define NUM_VERTS 4

layout(vertices = NUM_VERTS) out;

layout(location = 0) flat in uint mvpOffset[];
layout(location = 1) flat in uint heightmapIdx[];
layout(location = 2) in vec2 texUV[];
layout(location = 0) flat out uint outMvpOffset[NUM_VERTS];
layout(location = 1) flat out uint outHeightmapIdx[NUM_VERTS];
layout(location = 2) out vec2 outTexUV[NUM_VERTS];

void main() {
	outMvpOffset[gl_InvocationID] = mvpOffset[0];
	outHeightmapIdx[gl_InvocationID] = heightmapIdx[0];
	outTexUV[gl_InvocationID] = texUV[gl_InvocationID];
	gl_TessLevelInner[0] = 1.0;
	gl_TessLevelInner[1] = 1.0;
	gl_TessLevelOuter[0] = 1.0;
	gl_TessLevelOuter[1] = 1.0;
	gl_TessLevelOuter[2] = 1.0;
	gl_TessLevelOuter[3] = 1.0;
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}