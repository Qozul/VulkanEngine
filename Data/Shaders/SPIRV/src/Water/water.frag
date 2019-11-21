#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

struct Params {
	mat4 model;
	vec4 baseColour; // .w = displacementOffset
	vec4 tipColour; // .w = maxHeight
};

struct Material {
	uint displacementmapIdx;
	uint normalmapIdx;
};

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;

layout(location = 0) out vec4 fragColor;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec3 normal;
layout (location = 3) flat in int instanceIndex;
layout (location = 4) in vec4 shadowCoord;
layout (location = 5) flat in uint shadowMapIdx;
layout (location = 6) in float height;
layout (location = 7) flat in uint disp;

layout(set = 1, binding = 2) uniform sampler2D texSamplers[];

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = 1,  binding = 1) uniform samplerCube Cubemap;

layout(set = 0, binding = 1) readonly buffer ParamsData
{
	Params params[];
};

void main() {
	Params param = params[SC_PARAMS_OFFSET + instanceIndex];
	vec3 incident = normalize(lightPositions[0].xyz - worldPos);
	vec3 viewDir = normalize(cameraPosition.xyz - worldPos);
	vec3 halfDir = normalize(incident + viewDir);
	float dist = length(lightPositions[0].xyz - worldPos);

	vec3 distortion = texture(texSamplers[nonuniformEXT(disp)], texUV).rgb * 0.15;
	
	vec3 R = reflect(viewDir, normal);
	R += distortion;
	vec4 environmentCol = texture(Cubemap, R);
	
	float lambert = max(0.0, dot(incident, normal));
	float rFactor = max(0.0, dot(halfDir, normal));
	float sFactor = pow(rFactor , 5.0);
	
	vec3 albedoColour = mix(param.baseColour.xyz, param.tipColour.xyz, pow(height, 2.0));
	vec3 ambient = albedoColour.rgb * ambientColour.rgb;
	vec3 diffuse = max(param.baseColour.xyz * lambert, ambient);
	vec3 specular = vec3(1.0) * sFactor * 0.05;
	
	float shadow = projectShadow(shadowCoord / shadowCoord.w, vec2(0.0), shadowMapIdx);
	fragColor = vec4((mix(environmentCol.rgb, diffuse, 0.5) * shadow + specular), 1.0);
	fragColor = fragColor / (fragColor + vec4(1.0, 1.0, 1.0, 0.0));
	fragColor.rgb = pow(fragColor.rgb, vec3(1.0/2.2));
}
