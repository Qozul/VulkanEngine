#version 450

struct Params {
	mat4 model;
	vec4 tint; // tint.w = tileLength
};

layout(constant_id = 0) const uint SC_MVP_OFFSET = 0;
layout(constant_id = 1) const uint SC_PARAMS_OFFSET = 0;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in float inScale[];
layout(location = 2) in vec2 inTexOffset[];
layout(location = 3) flat in int inInstanceIndex[];
layout(location = 4) flat in vec4 inCameraPos[];

layout(location = 0) out vec2 outUvCoords;
layout(location = 1) flat out int outInstanceIndex;

const vec3 UP = vec3(0.0, 1.0, 0.0);

layout (set = 0, binding = 0) readonly buffer MVPs {
	mat4[] mvps;
};

layout (set = 0, binding = 1) readonly buffer ShaderParams {
	Params[] params;
};

void createVertex(vec3 right, vec2 uv, float r, float u, mat4 mvp)
{
	outInstanceIndex = inInstanceIndex[0];
	vec4 pos = vec4(inPosition[0] + (inScale[0] * (r * right + u * UP)), 1.0);
	outUvCoords = uv;
	gl_Position = mvp * pos;
	EmitVertex();
}

void main()
{
	Params parameters = params[SC_PARAMS_OFFSET + inInstanceIndex[0]];
	mat4 mvp = mvps[SC_MVP_OFFSET + inInstanceIndex[0]];
	// Need to calculate the billboarding in model space so that any model rotation is applied correctly
	vec3 modelSpaceBillboardPoint = (inverse(parameters.model) * vec4(inCameraPos[0].xyz, 1.0)).xyz;
	vec3 right = cross(normalize(modelSpaceBillboardPoint - inPosition[0]), UP);
	
	createVertex(right, inTexOffset[0],	-0.5, -0.5, mvp);
	createVertex(right, vec2(inTexOffset[0].x, inTexOffset[0].y + parameters.tint.w), -0.5,  0.5, mvp);
	createVertex(right, vec2(inTexOffset[0].x + parameters.tint.w, inTexOffset[0].y),  0.5, -0.5, mvp);
	createVertex(right, inTexOffset[0] + parameters.tint.w, 0.5, 0.5, mvp);
	
	EndPrimitive();
}

