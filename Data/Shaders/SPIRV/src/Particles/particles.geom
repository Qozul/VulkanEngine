#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in float inScale[];
layout(location = 2) in vec2 inTexOffset[];

layout(location = 0) out vec2 outUvCoords;

layout(push_constant) uniform PushConstants {
	mat4 mvp;
	vec3 cameraPosition;
	float tileLength;
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
	vec3 right = cross(normalize(inPosition[0] - PC.cameraPosition), UP);
	
	createVertex(right, inTexOffset[0],	-0.5, -0.5);
	createVertex(right, vec2(inTexOffset[0].x, inTexOffset[0].y + PC.tileLength), -0.5,  0.5);
	createVertex(right, vec2(inTexOffset[0].x + PC.tileLength, inTexOffset[0].y),  0.5, -0.5);
	createVertex(right, inTexOffset[0] + PC.tileLength, 0.5, 0.5);
	
	EndPrimitive();
}
