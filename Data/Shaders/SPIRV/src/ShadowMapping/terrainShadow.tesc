#version 450

#define NUM_VERTS 4

layout(vertices = NUM_VERTS) out;

layout(location = 0) flat in uint mvpOffset[];
layout(location = 0) flat out uint outMvpOffset[NUM_VERTS];

void main() {
	outMvpOffset[gl_InvocationID] = mvpOffset[0];
	gl_TessLevelInner[0] = 1.0;
	gl_TessLevelInner[1] = 1.0;
	gl_TessLevelOuter[0] = 1.0;
	gl_TessLevelOuter[1] = 1.0;
	gl_TessLevelOuter[2] = 1.0;
	gl_TessLevelOuter[3] = 1.0;
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}