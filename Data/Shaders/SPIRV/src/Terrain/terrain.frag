#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout (location = 0) in vec2 texUV;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 worldPos;

const vec3 kDiffuseColour = vec3(1.0, 1.0, 1.0);
const vec3 kSpecularColour = vec3(1.0, 1.0, 1.0);
const float kSpecularExponent = 0.0;

layout(set = 1, binding = 1) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = 0, binding = 3) uniform sampler2D texSampler;

void main() {
	vec3 incident = normalize ( lightPositions[0].xyz - worldPos );
	vec3 viewDir = normalize ( cameraPosition.xyz - worldPos );
	vec3 halfDir = normalize ( incident + viewDir );
	float dist = length(lightPositions[0].xyz - worldPos);
	//float atten = 1.0 - clamp ( dist / lightRadius , 0.0 , 1.0);
	float lambert = max(0.0, dot(incident, normal));
	float rFactor = max(0.0, dot(halfDir, normal));
	float sFactor = pow(rFactor , kSpecularExponent);
	
	vec4 texColour = texture(texSampler, texUV);
	
	vec3 ambient = texColour.rgb * ambientColour.xyz;
	vec3 diffuse = texColour.rgb * kDiffuseColour * lambert;
	vec3 specular = kSpecularColour * sFactor;
	fragColor = vec4(ambient + diffuse + specular, texColour.a);
}
