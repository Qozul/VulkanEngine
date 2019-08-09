#version 450

struct ElementData {
	mat4 model;
    mat4 mvp;
};

layout(triangles, equal_spacing, cw) in;
layout(location = 0) out vec3 outPos;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    ElementData uElementData;
} ubo;

void main(void)
{
	vec4 position = gl_in[0].gl_Position * gl_TessCoord.x +
					gl_in[1].gl_Position * gl_TessCoord.y +
					gl_in[2].gl_Position * gl_TessCoord.z;
	gl_Position = ubo.uElementData.mvp * position;
	outPos = position.xyz;
}
