#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout (constant_id = 0) const uint SC_COLOUR_TEXTURE_IDX = 0;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

void main()
{
	colour = texture(texSamplers[nonuniformEXT(SC_COLOUR_TEXTURE_IDX)], uv);
}