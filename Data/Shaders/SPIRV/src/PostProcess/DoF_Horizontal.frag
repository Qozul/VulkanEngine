// Reverse-mapped z-buffer depth of field using naive 9-tap 2-pass separated gaussian blur
#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout(location = 0) out vec4 outColour;

layout(location = 0) in vec2 inUV;

layout(push_constant) uniform PushConstants {
	uint colourIdx;
	uint depthIdx;
	uint shadowDepthIdx;
	float farZ;
	float nearZ;
	float screenX;
	float screenY;
} PC;

const float weights[5] = float[](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);
							
vec4 horizontalGaussian() 
{
	vec2 texelSize = 1.0 / vec2(textureSize(texSamplers[nonuniformEXT(PC.colourIdx)], 0));
	vec4 result = vec4(0.0);
	for (int x = -4; x <= 4; ++x) {
		vec2 off = vec2(float(x), 0.0) * texelSize;
		result += texture(texSamplers[nonuniformEXT(PC.colourIdx)], inUV + off) * weights[abs(x)];
	}
	return result;
}

void main()
{
	float focusZ = clamp(linearizeDepth(texture(texSamplers[nonuniformEXT(PC.depthIdx)], vec2(0.5)).r, PC.nearZ, PC.farZ), 0.0, 1.0);
	float currentZ = clamp(linearizeDepth(texture(texSamplers[nonuniformEXT(PC.depthIdx)], inUV).r, PC.nearZ, PC.farZ), 0.0, 1.0);
	
	float linearFactor = currentZ < focusZ ? 1.0 - (currentZ / focusZ) : (currentZ - focusZ) / (1.0 - focusZ);
	
	vec4 blurred = horizontalGaussian();
	outColour = mix(texture(texSamplers[nonuniformEXT(PC.colourIdx)], inUV), blurred, linearFactor + linearFactor);
}
