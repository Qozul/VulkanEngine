#version 450

#define NUM_VERTS 4

layout(vertices = NUM_VERTS) out;

layout (set = 0, binding = 4) uniform TessellationInfo {
	float patchRadius;
	float tessellationWeight;
	float padding0, padding1;
	vec4 frustumPlanes[6];
};

const float minWeight = 1.0;

bool checkCulling()
{
	vec4 pos = gl_in[gl_InvocationID].gl_Position;
	pos.y -= textureLod(heightmap, iTexUV[0], 0.0).r * maxHeight;
	for (int i = 0; i < 6; ++i) {
		if (dot(pos, frustumPlanes[i]) + patchRadius < 0.0) {
			return false;
		}
	}
	return true;
}

void main()
{	
	if (gl_InvocationID == 0) {
		if (!checkCulling()) {
			gl_TessLevelInner[0] = 0.0;
			gl_TessLevelInner[1] = 0.0;
			gl_TessLevelOuter[0] = 0.0;
			gl_TessLevelOuter[1] = 0.0;
			gl_TessLevelOuter[2] = 0.0;
			gl_TessLevelOuter[3] = 0.0;
		}
		else {
			gl_TessLevelInner[0] = tessellationWeight;
			gl_TessLevelInner[1] = tessellationWeight;
			gl_TessLevelOuter[0] = tessellationWeight;
			gl_TessLevelOuter[1] = tessellationWeight;
			gl_TessLevelOuter[2] = tessellationWeight;
			gl_TessLevelOuter[3] = tessellationWeight;
		}
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
