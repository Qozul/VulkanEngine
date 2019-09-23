/*#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in float inScale[];
layout(location = 2) in vec2 inTexOffset[];

layout(location = 0) out vec2 outUvCoords;
layout(location = 1) out vec3 debugColour;

layout(push_constant) uniform PushConstants {
	mat4 mvp;
	vec3 billboardPoint;
	float tileLength;
	vec3 systemPoint;
	float padding;
} PC;

const vec3 UP = vec3(0.0, 1.0, 0.0);

void createVertex(vec3 right, vec2 uv, float r, float u)
{
	vec3 pos = inPosition[0] + (inScale[0] * (r * right + u * UP));
	outUvCoords = uv;
	gl_Position = PC.mvp * vec4(pos, 1.0);
	EmitVertex();
}

void main()
{
	vec3 right = -cross(normalize(PC.systemPoint - PC.billboardPoint), UP);
	//vec3 right = vec3(1.0, 0.0, 0.0);
	createVertex(right, inTexOffset[0],	-0.5, -0.5);
	createVertex(right, vec2(inTexOffset[0].x, inTexOffset[0].y + PC.tileLength), -0.5,  0.5);
	createVertex(right, vec2(inTexOffset[0].x + PC.tileLength, inTexOffset[0].y),  0.5, -0.5);
	createVertex(right, inTexOffset[0] + PC.tileLength, 0.5, 0.5);
	
	EndPrimitive();
}*/
#version 450

struct PerInstanceParams {
	mat4 model;
	mat4 mvp;
	vec4 tint;
};

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in float inScale[];
layout(location = 2) in vec2 inTexOffset[];
layout(location = 3) flat in int inInstanceIndex[];

layout(location = 0) out vec2 outUvCoords;
layout(location = 1) flat out int outInstanceIndex;

layout(constant_id = 0) const int SC_MAX_PARTICLE_SYSTEMS = 111;
const vec3 UP = vec3(0.0, 1.0, 0.0);

layout(push_constant) uniform PushConstants {
	vec3 billboardPoint;
	float tileLength;
} PC;

layout (binding = 0) uniform Params {
	PerInstanceParams[SC_MAX_PARTICLE_SYSTEMS] params;
} UBO_0;

void createVertex(vec3 right, vec2 uv, float r, float u)
{
	vec4 pos = vec4(inPosition[0] + (inScale[0] * (r * right + u * UP)), 1.0);
	outUvCoords = uv;
	gl_Position = UBO_0.params[inInstanceIndex[0]].mvp * pos;
	EmitVertex();
}

void main()
{
	// Need to calculate the billboarding in model space so that any model rotation is applied correctly
	vec3 modelSpaceBillboardPoint = (inverse(UBO_0.params[inInstanceIndex[0]].model) * vec4(PC.billboardPoint, 1.0)).xyz;
	vec3 right = cross(normalize(modelSpaceBillboardPoint - inPosition[0]), UP);
	
	outInstanceIndex = inInstanceIndex[0];
	createVertex(right, inTexOffset[0],	-0.5, -0.5);
	createVertex(right, vec2(inTexOffset[0].x, inTexOffset[0].y + PC.tileLength), -0.5,  0.5);
	createVertex(right, vec2(inTexOffset[0].x + PC.tileLength, inTexOffset[0].y),  0.5, -0.5);
	createVertex(right, inTexOffset[0] + PC.tileLength, 0.5, 0.5);
	
	EndPrimitive();
}

