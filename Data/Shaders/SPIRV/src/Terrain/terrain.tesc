#version 450

//     3
//     0
// 0 1   1 2
//     0
//     1

#define NUM_VERTS 4

layout(vertices = NUM_VERTS) out;

layout (location = 0) in vec2 iTexUV[];

layout (location = 0) out vec2 outTexUV[NUM_VERTS];

const float maxWeight = 64.0;
const float minWeight = 1.0;

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

void main()
{
	float dist0 = 1.0 / distance(cameraPosition.xyz, gl_in[0].gl_Position.xyz);
	float dist1 = 1.0 / distance(cameraPosition.xyz, gl_in[1].gl_Position.xyz);
	float dist2 = 1.0 / distance(cameraPosition.xyz, gl_in[2].gl_Position.xyz);
	float dist3 = 1.0 / distance(cameraPosition.xyz, gl_in[3].gl_Position.xyz);
	
	gl_TessLevelOuter[0] = clamp(max(dist0, dist3) * maxWeight, minWeight, maxWeight);
	gl_TessLevelOuter[1] = clamp(max(dist0, dist1) * maxWeight, minWeight, maxWeight);
	gl_TessLevelOuter[2] = clamp(max(dist1, dist2) * maxWeight, minWeight, maxWeight);
	gl_TessLevelOuter[3] = clamp(max(dist2, dist3) * maxWeight, minWeight, maxWeight);
	
	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
	
	outTexUV[gl_InvocationID] = iTexUV[gl_InvocationID];
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
