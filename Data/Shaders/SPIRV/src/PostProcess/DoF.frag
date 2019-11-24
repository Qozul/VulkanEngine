// Reverse-mapped z-buffer depth of field
// See https://developer.nvidia.com/gpugems/GPUGems/gpugems_ch23.html for reference
#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout (constant_id = 0) const float SC_NEAR_Z = 0.1;
layout (constant_id = 1) const float SC_FAR_Z = 1000.0;
layout (constant_id = 2) const uint SC_GEOMETRY_COLOUR_IDX = 0;
layout (constant_id = 3) const uint SC_GEOMETRY_DEPTH_IDX = 0;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColour;

const float aperture = 0.5;
const float focalLength = 10; // 0.1 - 100

// Adapted from https://github.com/Unity-Technologies/PostProcessing/blob/v2/PostProcessing/Shaders/Builtins/DiskKernels.hlsl
const int kernelSampleCount = 22;
const vec2 kernel[kernelSampleCount] = {
	vec2(0, 0),
	vec2(0.53333336, 0),
	vec2(0.3325279, 0.4169768),
	vec2(-0.11867785, 0.5199616),
	vec2(-0.48051673, 0.2314047),
	vec2(-0.48051673, -0.23140468),
	vec2(-0.11867763, -0.51996166),
	vec2(0.33252785, -0.4169769),
	vec2(1, 0),
	vec2(0.90096885, 0.43388376),
	vec2(0.6234898, 0.7818315),
	vec2(0.22252098, 0.9749279),
	vec2(-0.22252095, 0.9749279),
	vec2(-0.90096885, 0.43388382),
	vec2(-0.62349, 0.7818314),
	vec2(-1, 0),
	vec2(-0.90096885, -0.43388376),
	vec2(-0.6234896, -0.7818316),
	vec2(-0.22252055, -0.974928),
	vec2(0.2225215, -0.9749278),
	vec2(0.6234897, -0.7818316),
	vec2(0.90096885, -0.43388376)
};

float objectDistance(float z)
{
	return -SC_FAR_Z * SC_NEAR_Z / (z * SC_FAR_Z - SC_NEAR_Z) - SC_FAR_Z);
}

void main()
{
	float planeInFocus = texture(texSamplers[nonuniformEXT(SC_GEOMETRY_DEPTH_IDX)], vec2(0.5)).r;
	float dist = objectDistance(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_DEPTH_IDX)], inUV).r);
	float CoCScale = (aperture * focalLength * planeInFocus * (SC_FAR_Z - SC_NEAR_Z)) / 
		((planeInFocus - focalLength) * SC_NEAR_Z * SC_FAR_Z);
	float CoCBias = (aperture * focalLength * (SC_NEAR_Z - planeInFocus)) /
		((planeInFocus * focalLength) * SC_NEAR_Z);
	float CoC =clamp(abs(z * CoCScale + CoCBias), -1.0, 1.0);
}
