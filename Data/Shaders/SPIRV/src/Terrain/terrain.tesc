#version 450

//     3
//     0
// 0 1   1 2
//     0
//     1

#define NUM_VERTS 4
#define NUM_DIST_BOUNDARIES 6

layout(vertices = NUM_VERTS) out;

layout (location = 0) in vec2 iTexUV[];

layout (location = 0) out vec2 outTexUV[NUM_VERTS];

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

const float maxWeight = 64.0;
const float minWeight = 1.0;
const float distanceBoundaries[NUM_DIST_BOUNDARIES] = float[](
	500.0, 400.0, 300.0, 200.0, 100.0, 50.0
);
const float tessellationLevelAddition[NUM_DIST_BOUNDARIES] = float[](
	1.0, 2.0, 4.0, 8.0, 16.0, 32.0
);

void main()
{
	// Old code
	/*float dist0 = 1.0 / distance(cameraPosition.xyz, gl_in[0].gl_Position.xyz);
	float dist1 = 1.0 / distance(cameraPosition.xyz, gl_in[1].gl_Position.xyz);
	float dist2 = 1.0 / distance(cameraPosition.xyz, gl_in[2].gl_Position.xyz);
	float dist3 = 1.0 / distance(cameraPosition.xyz, gl_in[3].gl_Position.xyz);
	
	gl_TessLevelOuter[0] = clamp(max(dist0, dist3) * maxWeight, minWeight, maxWeight);
	gl_TessLevelOuter[1] = clamp(max(dist0, dist1) * maxWeight, minWeight, maxWeight);
	gl_TessLevelOuter[2] = clamp(max(dist1, dist2) * maxWeight, minWeight, maxWeight);
	gl_TessLevelOuter[3] = clamp(max(dist2, dist3) * maxWeight, minWeight, maxWeight);*/
	
	// New code. Places each level on a flat amount depending on the distance by adding
	// together the levels for each lower distance band
	float dist0 = distance(cameraPosition.xyz, gl_in[0].gl_Position.xyz);
	float dist1 = distance(cameraPosition.xyz, gl_in[1].gl_Position.xyz);
	float dist2 = distance(cameraPosition.xyz, gl_in[2].gl_Position.xyz);
	float dist3 = distance(cameraPosition.xyz, gl_in[3].gl_Position.xyz);
	
	float dists[NUM_VERTS] = float[](
		max(dist0, dist3),
		max(dist0, dist1),
		max(dist1, dist2),
		max(dist2, dist3)
	);
	
	gl_TessLevelOuter[0] = minWeight;
	gl_TessLevelOuter[1] = minWeight;
	gl_TessLevelOuter[2] = minWeight;
	gl_TessLevelOuter[3] = minWeight;
	
	for (int i = 0; i < NUM_VERTS; ++i) {
		for (int j = 0; j < NUM_DIST_BOUNDARIES; ++j) {
			gl_TessLevelOuter[i] += step(dists[i], distanceBoundaries[j]) * tessellationLevelAddition[j];
		}
	}
	
	gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
	gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
	
	outTexUV[gl_InvocationID] = iTexUV[gl_InvocationID];
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
