#version 450
#extension GL_GOOGLE_include_directive : enable
#define USE_LIGHTS_UBO
#include "../common.glsl"

layout(constant_id = 0) const uint SC_G_BUFFER_POSITIONS_IDX = 0;
layout(constant_id = 1) const uint SC_G_BUFFER_NORMALS_IDX = 0;
layout(constant_id = 2) const uint SC_SHADOW_DEPTH_IDX = 0;
layout(constant_id = 3) const uint SC_G_BUFFER_DEPTH_IDX = 0;

const int PCF_COUNT = 2;
const int TOTAL_PCF_COUNT = (PCF_COUNT + PCF_COUNT + 1) * (PCF_COUNT + PCF_COUNT + 1);

layout(location = 0) flat in uint inInstanceIndex;
layout(location = 1) flat in vec4 inCameraPos;
layout(location = 2) flat in mat4 inShadowMatrix;

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;
layout(location = 2) out vec4 outAmbient;

layout(push_constant) uniform PushConstants {
	layout(offset = 96) float screenWidth;
	layout(offset = 100) float screenHeight;
	layout(offset = 104) float screenX;
	layout(offset = 108) float screenY;
} PC;

float pcfShadow(vec4 shadowCoord)
{
	ivec2 texSize = textureSize(texSamplers[nonuniformEXT(SC_SHADOW_DEPTH_IDX)], 0);
	float dx = 1.0 / texSize.x;
	float dy = 1.0 / texSize.y;
	float shadow = 0;
	for (int x = -PCF_COUNT; x <= PCF_COUNT; ++x) {
		for (int y = -PCF_COUNT; y <= PCF_COUNT; ++y) {
			shadow += projectShadow(shadowCoord, vec2(dx*x,dy*y), SC_SHADOW_DEPTH_IDX);
		}
	}
	return shadow / TOTAL_PCF_COUNT;
}

void main()
{
	Light light = lights[inInstanceIndex];
	vec3 lightPos = light.position;
	
	vec2 uv = vec2(gl_FragCoord.x * (1.0 / PC.screenWidth) + PC.screenX, (gl_FragCoord.y) * (1.0 / PC.screenHeight));
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
	float shadow = pcfShadow(shadowCoord / shadowCoord.w);
	outDiffuse = vec4(light.colour * lambert * attenuation * shadow, worldPos.w);
	outSpecular = vec4(light.colour * sf * attenuation * 0.33, 1.0);
}
