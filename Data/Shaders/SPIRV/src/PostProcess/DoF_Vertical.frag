// Reverse-mapped z-buffer depth of field
// See https://developer.nvidia.com/gpugems/GPUGems/gpugems_ch23.html and 
// https://catlikecoding.com/unity/tutorials/advanced-rendering/depth-of-field/ for reference
#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout (constant_id = 1) const float SC_NEAR_Z = 0.1;
layout (constant_id = 2) const float SC_FAR_Z = 2500.0;
layout (constant_id = 0) const uint SC_GEOMETRY_COLOUR_IDX = 0;
layout (constant_id = 3) const uint SC_GEOMETRY_DEPTH_IDX = 0;

layout(location = 0) out vec4 outColour;

layout(location = 0) in vec2 inUV;

const float weights[5] = float[](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);
				
vec4 verticalGaussian() 
{
	vec2 texelSize = 1.0 / vec2(textureSize(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], 0));
	vec4 result = vec4(0.0);
	for (int y = -4; y <= 4; ++y) {
		vec2 off = vec2(0.0, float(y)) * texelSize;
		result += texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], inUV + off) * weights[abs(y)];
	}
	return result;
}

void main()
{
	float focusZ = clamp(linearizeDepth(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_DEPTH_IDX)], vec2(0.5)).r, SC_NEAR_Z, SC_FAR_Z), 0.0, 1.0);
	float currentZ = clamp(linearizeDepth(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_DEPTH_IDX)], inUV).r, SC_NEAR_Z, SC_FAR_Z), 0.0, 1.0);
	
	float linearFactor = currentZ < focusZ ? 1.0 - (currentZ / focusZ) : (currentZ - focusZ) / (1.0 - focusZ);
	
	vec4 blurred = verticalGaussian();
	outColour = mix(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], inUV), blurred, linearFactor + linearFactor);
}
