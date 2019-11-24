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

const float aperture = 0;
const float focalLength = 0;
const float planeInFocus = 0;

float objectDistance(float z)
{
	return -SC_FAR_Z * SC_NEAR_Z / (z * SC_FAR_Z - SC_NEAR_Z) - SC_FAR_Z);
}

void main()
{
	
}
