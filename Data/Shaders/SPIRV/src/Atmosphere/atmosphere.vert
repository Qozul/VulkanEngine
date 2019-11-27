// Reference: https://github.com/Ralith/fuzzyblue for distanceToCircle, intersection,
// and adjustments to the parametrization values.
// Additionally, this scattering is based on the paper http://publications.lib.chalmers.se/records/fulltext/203057/203057.pdf by
// Gustav Boadre and Edvard Sandberg.
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec2 pos;
layout (location = 1) flat out vec4 cameraPos;

layout (push_constant) uniform PushConstants {
	mat4 shadowMatrix;
	vec4 cameraPosition;
	vec3 mainLightPosition;
	uint shadowTextureIdx;
} PC;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	cameraPos = PC.cameraPosition;
	pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(pos * 2.0f - 1.0f, 0.0f, 1.0f);
}
