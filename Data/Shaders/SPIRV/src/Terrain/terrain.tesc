// Largely based on https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/terraintessellation/terrain.tesc but 
// basing TL on distance of a side from the camera for LOD
#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"
#include "terrain_structs.glsl"

#define NUM_VERTS 4

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout(vertices = NUM_VERTS) out;

layout (location = 0) in vec2 iTexUV[];
layout (location = 1) flat in int instanceIndex[];
layout (location = 2) in vec4 shadowCoord[];
layout (location = 3) flat in uint shadowMapIdx[];
layout (location = 4) in vec3 normal[];

layout (location = 0) out vec2 outTexUV[NUM_VERTS];
layout (location = 1) flat out int outInstanceIndex[NUM_VERTS];
layout (location = 2) out vec4 outShadowCoord[NUM_VERTS];
layout (location = 3) flat out uint outShadowMapIdx[NUM_VERTS];
layout (location = 4) out vec3 outNormal[NUM_VERTS];

layout(set = GLOBAL_SET, binding = LIGHT_UBO_BINDING) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer ParamsData
{
	Params params[];
};

layout(set = COMMON_SET, binding = COMMON_MATERIALS_BINDING) readonly buffer TexIndices
{
	TextureIndices textureIndices[];
};

const float minWeight = 1.0;

float calculateTessLevel(float d0, float d1, in Params parameters)
{
	// Get closest vertex on the side to the camera
	float dist = min(d0, d1) - parameters.closeDistance;
	// Calculate a continuous value between 0 and 1
	float proportion = clamp(dist/parameters.distanceFarMinusClose, 0.0, 1.0);
	return mix(parameters.maxTessellationWeight, minWeight, proportion);
}

bool checkCulling(in Params parameters)
{
	TextureIndices texIndices = textureIndices[SC_MATERIAL_OFFSET + instanceIndex[0]];
	vec4 pos = gl_in[gl_InvocationID].gl_Position;
	//pos.y -= texture(texSamplers[nonuniformEXT(texIndices.heightmapIdx)], inNormalizedUvs[0]).r * inMaxHeight[0];
	for (int i = 0; i < 6; ++i) {
		if (dot(pos, parameters.frustumPlanes[i]) + parameters.patchRadius < 0.0) {
			return false;
		}
	}
	return true;
}

vec3 getVertexPosition(int i)
{
	TextureIndices texIndices = textureIndices[SC_MATERIAL_OFFSET + instanceIndex[0]];
	vec3 pos = gl_in[i].gl_Position.xyz;
	//pos.y -= texture(texSamplers[nonuniformEXT(texIndices.heightmapIdx)], inNormalizedUvs[i]).r * inMaxHeight[0];
	return pos;
}

void main()
{
	Params parameters = params[SC_PARAMS_OFFSET + instanceIndex[0]];
	outInstanceIndex[gl_InvocationID] = instanceIndex[0];
	outShadowMapIdx[gl_InvocationID] = shadowMapIdx[0];
	// Only calculate per-patch stuff (tess levels) once per patch
	if (gl_InvocationID == 0) {
		if (!checkCulling(parameters)) {
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
			
			gl_TessLevelOuter[0] = calculateTessLevel(dists[0], dists[3], parameters);
			gl_TessLevelOuter[1] = calculateTessLevel(dists[0], dists[1], parameters);
			gl_TessLevelOuter[2] = calculateTessLevel(dists[1], dists[2], parameters);
			gl_TessLevelOuter[3] = calculateTessLevel(dists[2], dists[3], parameters);
			
			gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
			gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
		}
	}
	outTexUV[gl_InvocationID] = iTexUV[gl_InvocationID];
	outShadowCoord[gl_InvocationID] = shadowCoord[gl_InvocationID];
	outNormal[gl_InvocationID] = normal[gl_InvocationID];
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
