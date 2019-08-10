#version 450

#define NUM_VERTS 3

layout(vertices = NUM_VERTS) out;
/*
layout (set = 0, binding = 4) uniform TessellationInfo {
	float patchRadius;
	float tessellationWeight;
	float padding0, padding1;
	vec4 frustumPlanes[6];
};*/

const float minWeight = 1.0;
/*
bool checkCulling()
{
	vec4 pos = gl_in[gl_InvocationID].gl_Position;
	for (int i = 0; i < 6; ++i) {
		if (dot(pos, frustumPlanes[i]) + patchRadius < 0.0) {
			return false;
		}
	}
	return true;
}*/

void main()
{	
	if (gl_InvocationID == 0) {
		//if (!checkCulling()) {
			gl_TessLevelInner[0] = 0.0;
			gl_TessLevelOuter[0] = 0.0;
			gl_TessLevelOuter[1] = 0.0;
			gl_TessLevelOuter[2] = 0.0;
		//}
		//else {
			gl_TessLevelInner[0] = 1;
			gl_TessLevelOuter[0] = 1;
			gl_TessLevelOuter[1] = 1;
			gl_TessLevelOuter[2] = 1;
		//}
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
