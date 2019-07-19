#version 440 core


in Vertex
{
	vec2 texUV;
	vec3 worldPos;
	vec3 normal;
} IN;

out vec4 fragColor;

uniform sampler2D tDiffuse;
uniform sampler2D tBonus;
uniform vec3 uCamPosition = vec3(0.0, 0.0, 10.0);

const vec3 kLightPosition = vec3(1000.0, 500.0, -1000.0);
const vec3 kAmbientColour = vec3(0.2, 0.2, 0.2);
const vec3 kDiffuseColour = vec3(1.0, 1.0, 1.0);
const vec3 kSpecularColour = vec3(1.0, 1.0, 1.0);
const float kSpecularExponent = 0.5;

void main(void)
{	
	vec3 incident = normalize ( kLightPosition - IN.worldPos );
	vec3 viewDir = normalize ( uCamPosition - IN.worldPos );
	vec3 halfDir = normalize ( incident + viewDir );
	float dist = length(kLightPosition - IN.worldPos);
	//float atten = 1.0 - clamp ( dist / lightRadius , 0.0 , 1.0);
	float lambert = max(0.0, dot(incident, IN.normal));
	float rFactor = max(0.0, dot(halfDir, IN.normal));
	float sFactor = pow(rFactor , kSpecularExponent);
	
	vec4 texColour = texture(tDiffuse, IN.texUV);
	vec4 texColour2 = texture(tBonus, IN.texUV);
	
	vec3 ambient = texColour.rgb * kAmbientColour;
	vec3 diffuse = texColour.rgb * kDiffuseColour * lambert;
	vec3 specular = texColour2.rgb * sFactor * 0.05;
	fragColor = vec4(ambient + diffuse + specular, texColour.a);
}
