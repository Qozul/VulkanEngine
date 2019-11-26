#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(quads, equal_spacing, cw) in;

layout(location = 0) flat in uint mvpOffset[];

layout(set = 0, binding = 0) readonly buffer StorageBuffer {
    mat4[] data;
} mvps;

void main() {	
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	gl_Position = mvps.data[mvpOffset[0]] * position;
}