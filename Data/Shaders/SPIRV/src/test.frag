#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout (location = 0) in vec4 colour;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 worldPos;

const vec3 uCamPosition = vec3(0.0, 0.0, 10.0);

const vec3 kLightPosition = vec3(1000.0, 500.0, -1000.0);
const vec3 kAmbientColour = vec3(0.2, 0.2, 0.2);
const vec3 kDiffuseColour = vec3(1.0, 1.0, 1.0);
const vec3 kSpecularColour = vec3(1.0, 1.0, 1.0);
const float kSpecularExponent = 0.5;

void main() {
	vec3 incident = normalize ( kLightPosition - worldPos );
	vec3 viewDir = normalize ( uCamPosition - worldPos );
	vec3 halfDir = normalize ( incident + viewDir );
	float dist = length(kLightPosition - worldPos);
	//float atten = 1.0 - clamp ( dist / lightRadius , 0.0 , 1.0);
	
	float lambert = max(0.0, dot(incident, normal));
	float rFactor = max(0.0, dot(halfDir, normal));
	float sFactor = pow(rFactor , kSpecularExponent);
	
	vec3 ambient = colour.rgb * kAmbientColour;
	vec3 diffuse = colour.rgb * kDiffuseColour * lambert;
	vec3 specular = kSpecularColour * sFactor * 0.05;
	fragColor = vec4(ambient + diffuse + specular, colour.a);
}
