#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"
#include "terrain_structs.glsl"

layout(constant_id = 0) const uint SC_MVP_OFFSET = 0;
layout(constant_id = 1) const uint SC_PARAMS_OFFSET = 0;

const float FAR_GRASS = 500.0;
const float MAX_OFFSET = 0.1;

layout(triangles) in;
layout(triangle_strip, max_vertices = 13) out;

layout (location = 0) in vec2 inTexUV[];
layout (location = 1) in vec3 inPos[];
layout (location = 2) in vec3 inNormal[];
layout (location = 3) flat in int inInstanceIndex[];
layout (location = 4) flat in vec3 inCamPos[];

layout (location = 0) out vec2 outTexUV;
layout (location = 1) out vec3 outWorldPos;
layout (location = 2) out vec3 outNormal;
layout (location = 3) flat out int outInstanceIndex;
layout (location = 4) flat out vec3 outCamPos;
layout (location = 5) flat out int outGrass;

layout(set = COMMON_SET, binding = COMMON_MVP_BINDING) readonly buffer UniformBufferObject {
    mat4 mvps[];
};

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer ParamsData
{
	Params params[];
};

void passThrough(mat4 model)
{
	for(int i = 0; i < gl_in.length(); ++i) {
		gl_Position = gl_in[i].gl_Position;
		outTexUV = inTexUV[i];
		outWorldPos = (model * vec4(inPos[i], 1.0)).xyz;
		outNormal = inNormal[i];
		outInstanceIndex = inInstanceIndex[0];
		outCamPos = inCamPos[0];
		outGrass = -1;
		EmitVertex();
	}
	EndPrimitive();
}

void createVertex(vec3 right, vec2 uv, float r, float u, mat4 mvp, mat4 model, int idx)
{
	vec4 pos = vec4(inPos[idx].xyz + (2.0 * (r * right + u * inNormal[idx])), 1.0);
	outInstanceIndex = inInstanceIndex[0];
	outCamPos = inCamPos[0];
	outTexUV = uv;
	outWorldPos = (model * pos).xyz;
	outNormal = inNormal[idx];
	outGrass = idx;
	gl_Position = mvp * pos;
	EmitVertex();
}

void main() 
{
	Params parameters = params[SC_PARAMS_OFFSET + inInstanceIndex[0]];
	// Pass through terrain tri
	passThrough(parameters.model);
	
	// Generate new quad for grass if appropriate
	int idx = int(clamp(fract(sin(inNormal[0].x) * sin(inPos[2].y)) * 2.99, 0.1, 2.99));
	mat4 mvp = mvps[SC_MVP_OFFSET + inInstanceIndex[0]];
	float height = clamp((parameters.model * vec4(inPos[idx], 1.0)).y / parameters.heights.x, 0.0, 1.0);
	bool slope = inNormal[idx].y > inNormal[idx].x + 0.1 && inNormal[idx].y > inNormal[idx].z + 0.1;
	if (distance(inCamPos[0], inPos[idx]) < FAR_GRASS && slope &&
		height > parameters.heights.z && height < parameters.heights.w) {
		vec3 right = cross(normalize(vec3(inCamPos[0].x, inPos[idx].y, inCamPos[0].z) - inPos[idx]), inNormal[idx]);
		float waveStrength = sin(parameters.time);
		createVertex(right, vec2(0.0, 0.0),	-0.2, -0.1, mvp, parameters.model, idx); // 0
		createVertex(right, vec2(0.5, 1.0), waveStrength * MAX_OFFSET,  0.9, mvp, parameters.model, idx); // 1
		createVertex(right, vec2(1.0, 0.0), 0.2, -0.1, mvp, parameters.model, idx); // 3
		EndPrimitive();
	}
}
