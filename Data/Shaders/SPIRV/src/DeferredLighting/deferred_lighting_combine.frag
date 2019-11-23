#version 450
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout(constant_id = 0) const uint SC_DIFFUSE_IDX = 0;
layout(constant_id = 1) const uint SC_SPECULAR_IDX = 0;
layout(constant_id = 2) const uint SC_G_BUFFER_ALBEDO = 0;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColour;

const float AMBIENT = 0.1;

void main()
{
	vec4 diffuse = texture(texSamplers[nonuniformEXT(SC_DIFFUSE_IDX)], inUV);
	vec3 specular = texture(texSamplers[nonuniformEXT(SC_SPECULAR_IDX)], inUV).rgb;
	vec4 fullAlbedo = texture(texSamplers[nonuniformEXT(SC_G_BUFFER_ALBEDO)], inUV);
	vec3 albedo = fullAlbedo.rgb;
	float includeSpecular = fullAlbedo.a;
	
	outColour.rgb = max(diffuse.rgb * albedo, diffuse.rgb * AMBIENT);
	outColour.rgb += (specular * includeSpecular);
	outColour.a = diffuse.a;
	reinhardTonemap(outColour);
}
