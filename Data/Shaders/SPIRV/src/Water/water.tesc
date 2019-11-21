#version 450
#extension GL_EXT_nonuniform_qualifier : require

#define NUM_VERTS 4

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;

layout(vertices = NUM_VERTS) out;

layout (location = 0) in vec2 iTexUV[];
layout (location = 1) flat in int instanceIndex[];
layout (location = 2) in vec4 shadowCoord[];
layout (location = 3) flat in uint shadowMapIdx[];

layout (location = 0) out vec2 outTexUV[NUM_VERTS];
layout (location = 1) flat out int outInstanceIndex[NUM_VERTS];
layout (location = 2) out vec4 outShadowCoord[NUM_VERTS];
layout (location = 3) flat out uint outShadowMapIdx[NUM_VERTS];

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

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
	float dists[NUM_VERTS] = float[](
		distance(cameraPosition.xyz, gl_in[0].gl_Position.xyz),
		distance(cameraPosition.xyz, gl_in[1].gl_Position.xyz),
		distance(cameraPosition.xyz, gl_in[2].gl_Position.xyz),
		distance(cameraPosition.xyz, gl_in[3].gl_Position.xyz)
	);
	
	gl_TessLevelOuter[0] = calculateTessLevel(dists[0], dists[3]);
	gl_TessLevelOuter[1] = calculateTessLevel(dists[0], dists[1]);
	gl_TessLevelOuter[2] = calculateTessLevel(dists[1], dists[2]);
	gl_TessLevelOuter[3] = calculateTessLevel(dists[2], dists[3]);
	
	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
	outTexUV[gl_InvocationID] = iTexUV[gl_InvocationID];
	outShadowCoord[gl_InvocationID] = shadowCoord[gl_InvocationID];
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}