// Largely based on https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/terraintessellation/terrain.tesc but 
// basing TL on distance of a side from the camera for LOD
#version 450

#define NUM_VERTS 4

layout(vertices = NUM_VERTS) out;

layout (location = 0) in vec2 iTexUV[];

layout (location = 0) out vec2 outTexUV[NUM_VERTS];

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout (set = 0, binding = 2) uniform TessellationInfo {
	float distanceFarMinusClose;
	float closeDistance;
	float patchRadius;
	float maxTessellationWeight;
	vec4 frustumPlanes[6];
};

layout(set = 2, binding = 0) uniform sampler2D heightmap;

const float maxHeight = 100.0;
const float minWeight = 1.0;

float calculateTessLevel(float d0, float d1)
{
	// Get closest vertex on the side to the camera
	float dist = min(d0, d1) - closeDistance;
	// Calculate a continuous value between 0 and 1
	float proportion = clamp(dist/distanceFarMinusClose, 0.0, 1.0);
	return mix(maxTessellationWeight, minWeight, proportion);
}

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

vec3 getVertexPosition(int i)
{
	vec3 pos = gl_in[i].gl_Position.xyz;
	pos.y -= textureLod(heightmap, iTexUV[i], 0.0).r * maxHeight;
	return pos;
}

void main()
{	
	// Only calculate per-patch stuff (tess levels) once per patch
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
			float dists[NUM_VERTS] = float[](
				distance(cameraPosition.xyz, getVertexPosition(0)),
				distance(cameraPosition.xyz, getVertexPosition(1)),
				distance(cameraPosition.xyz, getVertexPosition(2)),
				distance(cameraPosition.xyz, getVertexPosition(3))
			);
			
			gl_TessLevelOuter[0] = calculateTessLevel(dists[0], dists[3]);
			gl_TessLevelOuter[1] = calculateTessLevel(dists[0], dists[1]);
			gl_TessLevelOuter[2] = calculateTessLevel(dists[1], dists[2]);
			gl_TessLevelOuter[3] = calculateTessLevel(dists[2], dists[3]);
			
			gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
			gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
		}
	}
	outTexUV[gl_InvocationID] = iTexUV[gl_InvocationID];
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
