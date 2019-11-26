#version 450
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout(constant_id = 0) const uint SC_DIFFUSE_IDX = 0;
layout(constant_id = 1) const uint SC_SPECULAR_IDX = 0;
layout(constant_id = 2) const uint SC_G_BUFFER_ALBEDO = 0;
layout(constant_id = 3) const uint SC_AMBIENT_OCCULSION = 0;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColour;

const float AMBIENT = 0.1;

float blurSSAO()
{
	vec2 texelSize = 1.0 / vec2(textureSize(texSamplers[nonuniformEXT(SC_AMBIENT_OCCULSION)], 0));
	float result = 0.0;
	for (int x = -2; x < 2; ++x) {
		for (int y = -2; y < 2; ++y) {
			vec2 off = vec2(float(x), float(y)) * texelSize;
			result += texture(texSamplers[nonuniformEXT(SC_AMBIENT_OCCULSION)], inUV + off).r;
		}
	}
	return result / 16.0;
}

void main()
{
	vec4 diffuse = texture(texSamplers[nonuniformEXT(SC_DIFFUSE_IDX)], inUV);
	vec3 specular = texture(texSamplers[nonuniformEXT(SC_SPECULAR_IDX)], inUV).rgb;
	vec4 fullAlbedo = texture(texSamplers[nonuniformEXT(SC_G_BUFFER_ALBEDO)], inUV);
	
	outColour.rgb = max(diffuse.rgb, AMBIENT * blurSSAO());
	outColour.rgb *= fullAlbedo.rgb;
	outColour.rgb += (specular);
	outColour.a = fullAlbedo.a;
	reinhardTonemap(outColour);
}
