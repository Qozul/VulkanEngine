#version 450

struct ElementData {
	mat4 model;
    mat4 mvp;
};

layout(triangles, equal_spacing, cw) in;
layout(location = 0) in vec2 uvcoords[];
layout(location = 0) out vec3 view_ray;
layout(location = 1) out vec2 oUvcoords;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 modelView;
    mat4 projection;
} ubo;

void main(void)
{
	vec4 position = gl_in[0].gl_Position * gl_TessCoord.x +
					gl_in[1].gl_Position * gl_TessCoord.y +
					gl_in[2].gl_Position * gl_TessCoord.z;
	vec4 finalPos = ubo.projection * ubo.modelView * position;
	gl_Position = finalPos;
	view_ray = (inverse(ubo.modelView) * vec4((inverse(ubo.projection) * finalPos).xyz, 0.0)).xyz;
	oUvcoords = uvcoords[0] * gl_TessCoord.x +
					uvcoords[1] * gl_TessCoord.y +
					uvcoords[2] * gl_TessCoord.z;
}
