#version 400 core

const int kInnerWeight = 64;
const int kOuterWeight = 64;

layout(vertices = 3) out;

in Vertex {
	vec2 texUV;
} IN[];

out Vertex {
	vec2 texUV;
} OUT[];
void main()
{
	gl_TessLevelInner[0] = kInnerWeight;
	gl_TessLevelOuter[0] = kOuterWeight;
	gl_TessLevelOuter[1] = kOuterWeight;
	gl_TessLevelOuter[2] = kOuterWeight;
	
	OUT[gl_InvocationID].texUV = IN[gl_InvocationID].texUV;
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
