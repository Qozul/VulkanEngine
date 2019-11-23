#version 450
#extension GL_EXT_nonuniform_qualifier : require

#define NUM_VERTS 4

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;

layout(vertices = NUM_VERTS) out;

layout(location = 0) in vec2 iTexUV[];
layout(location = 1) flat in int instanceIndex[];
layout(location = 2) flat in uint shadowMapIdx[];
layout(location = 3) flat in vec3 inCamPos[];
layout(location = 4) flat in mat4 shadowMat[];

layout(location = 0) out vec2 outTexUV[NUM_VERTS];
layout(location = 1) flat out int outInstanceIndex[NUM_VERTS];
layout(location = 2) flat out uint outShadowMapIdx[NUM_VERTS];
layout(location = 3) flat out vec3 outCamPos[NUM_VERTS];
layout(location = 4) flat out mat4 outShadowMat[NUM_VERTS];

float calculateTessLevel(float d0, float d1)
{
	// Get closest vertex on the side to the camera
	float dist = min(d0, d1) - 50.0;
	// Calculate a continuous value between 0 and 1
	float proportion = clamp(dist/500.0, 0.0, 1.0);
	return mix(1.0, 1.0, proportion);
}

void main()
{
	outInstanceIndex[gl_InvocationID] = instanceIndex[0];
	outShadowMapIdx[gl_InvocationID] = shadowMapIdx[0];
	outCamPos[gl_InvocationID] = inCamPos[0];
	float dists[NUM_VERTS] = float[](
		distance(inCamPos[0], gl_in[0].gl_Position.xyz),
		distance(inCamPos[0], gl_in[1].gl_Position.xyz),
		distance(inCamPos[0], gl_in[2].gl_Position.xyz),
		distance(inCamPos[0], gl_in[3].gl_Position.xyz)
	);
	
	gl_TessLevelOuter[0] = calculateTessLevel(dists[0], dists[3]);
	gl_TessLevelOuter[1] = calculateTessLevel(dists[0], dists[1]);
	gl_TessLevelOuter[2] = calculateTessLevel(dists[1], dists[2]);
	gl_TessLevelOuter[3] = calculateTessLevel(dists[2], dists[3]);
	
	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
	outTexUV[gl_InvocationID] = iTexUV[gl_InvocationID];
	outShadowMat[gl_InvocationID] = shadowMat[0];
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
