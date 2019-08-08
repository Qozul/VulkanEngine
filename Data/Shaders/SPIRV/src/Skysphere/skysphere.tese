#version 450

struct ElementData {
	mat4 model;
    mat4 mvp;
};

layout(quads, equal_spacing, cw) in;


layout(set = 0, binding = 0) uniform UniformBufferObject {
    ElementData uElementData;
} ubo;

layout(set = 0, binding = 0) uniform ScatteringData {

};

void main(void)
{
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	gl_Position = ubo.uElementData.mvp * position;
}
