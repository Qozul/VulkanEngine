#version 440 core
#extension GL_ARB_bindless_texture : require

struct Texture
{
	sampler2DArray handle;
	float page;
	sampler2DArray handle2;
	float page2;
};

in Vertex
{
	vec2 texUV;
	vec3 worldPos;
	vec3 normal;
	flat int instanceID;
} IN;

out vec4 fragColor;

layout(std430, binding = 2) readonly buffer Textures
{
    Texture[] tDiffuse;
};


uniform vec3 uCamPosition = vec3(0.0, 0.0, 10.0);

const vec3 kLightPosition = vec3(1000.0, 500.0, -1000.0);
const vec3 kAmbientColour = vec3(0.3, 0.3, 0.3);
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
	
	vec4 texColour = texture(tDiffuse[IN.instanceID].handle, vec3(IN.texUV, tDiffuse[IN.instanceID].page));
	vec4 texColour2 = texture(tDiffuse[IN.instanceID].handle2, vec3(IN.texUV, tDiffuse[IN.instanceID].page2));
	
	vec3 ambient = texColour.rgb * kAmbientColour;
	vec3 diffuse = texColour.rgb * kDiffuseColour * lambert;
	vec3 specular = texColour2.rgb * sFactor * 0.05;
	fragColor = vec4(ambient + diffuse + specular, texColour.a);
}
