#version 450

#define NUM_VERTS 3

layout(vertices = NUM_VERTS) out;
layout(location = 0) in vec2 uvcoords[];
layout(location = 0) out vec2 oUvcoords[NUM_VERTS];
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
		//}
		//else {
			gl_TessLevelInner[0] = 8.0;
			gl_TessLevelOuter[0] = 8.0;
			gl_TessLevelOuter[1] = 8.0;
			gl_TessLevelOuter[2] = 8.0;
		//}
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	oUvcoords[gl_InvocationID] = uvcoords[gl_InvocationID];
}
