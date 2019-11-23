#version 450
#extension GL_GOOGLE_include_directive : enable
#define USE_LIGHTS_UBO
#include "../common.glsl"

layout(constant_id = 0) const uint SC_G_BUFFER_POSITIONS_IDX = 0;
layout(constant_id = 1) const uint SC_G_BUFFER_NORMALS_IDX = 0;
layout(constant_id = 2) const float SC_INV_SCREEN_X = 0;
layout(constant_id = 3) const float SC_INV_SCREEN_Y = 0;
layout(constant_id = 4) const uint SC_SHADOW_DEPTH_IDX = 0;
layout(constant_id = 5) const uint SC_G_BUFFER_DEPTH_IDX = 0;

layout(location = 0) flat in uint inInstanceIndex;
layout(location = 1) flat in vec4 inCameraPos;
layout(location = 2) flat in mat4 inShadowMatrix;

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;

void main()
{
	Light light = lights[inInstanceIndex];
	vec3 lightPos = light.position;
	
	vec2 uv = vec2(gl_FragCoord.x * SC_INV_SCREEN_X, gl_FragCoord.y * SC_INV_SCREEN_Y);
	vec4 worldPos = texture(texSamplers[nonuniformEXT(SC_G_BUFFER_POSITIONS_IDX)], uv);
	vec4 rawNormal = texture(texSamplers[nonuniformEXT(SC_G_BUFFER_NORMALS_IDX)], uv);
	float specExponent = rawNormal.w;
	vec3 N = normalize(rawNormal.xyz * 2.0 - 1.0);
	
	float dist = length(lightPos - worldPos.xyz);
	float attenuation = (1.0 - clamp(dist / light.radius, 0.0, 1.0) * light.attenuationFactor);
	
	if (attenuation == 0.0) discard;
	
	vec3 I = normalize(lightPos - worldPos.xyz);
	vec3 V = normalize(inCameraPos.xyz - worldPos.xyz);
	vec3 H = normalize(I + V);
	
	float lambert = clamp(dot(I, N), 0.0, 1.0);
	float rf = clamp(dot(H, N), 0.0, 1.0);
	float sf = pow(rf, specExponent);
	
	vec4 shadowCoord = BIAS_MATRIX * inShadowMatrix * worldPos;
	float shadow = projectShadow(shadowCoord / shadowCoord.w, vec2(0.0), SC_SHADOW_DEPTH_IDX);
	outDiffuse = vec4(light.colour * lambert * attenuation * shadow, worldPos.w);
	outSpecular = vec4(light.colour * sf * attenuation * 0.33, 1.0);
}
