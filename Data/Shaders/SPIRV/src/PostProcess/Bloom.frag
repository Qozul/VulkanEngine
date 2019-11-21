#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout (constant_id = 0) const float SC_NEAR_Z = 0.1;
layout (constant_id = 1) const float SC_FAR_Z = 1000.0;
layout (constant_id = 2) const uint SC_GEOMETRY_COLOUR_IDX = 0;
layout (constant_id = 3) const uint SC_GEOMETRY_DEPTH_IDX = 0;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

void main()
{
	colour = texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv);
	//float depth = texture(texSamplers[nonuniformEXT(SC_GEOMETRY_DEPTH_IDX)], uv).r;
	//depth = clamp(linearizeDepth(depth, SC_NEAR_Z, SC_FAR_Z), 0.0, 1.0);
	//colour = vec4(depth, depth, depth, 1.0);
}
