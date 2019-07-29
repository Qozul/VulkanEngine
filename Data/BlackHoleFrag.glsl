#version 440

uniform samplerCube uEnvMap;

layout(std140, binding = 1) uniform UFragLightData
{
	vec4 uSpec; // .xyz = specular colour, .w = Ns
	vec4 uDiff; // .xyz = diffuse colour, .w = Nh
	vec4 uAmb; // .xyz = ambient colour, .w = padding
	vec4 uCamPosition;
	vec4 uLightPosition;
};

in Vertex
{
	vec3 worldPos;
	vec2 texUV;
	vec3 worldNormal;
	vec3 envCoords;
} IN;

out vec4 fragColor;

void main(void)
{	
	vec3 incident = normalize ( uLightPosition.xyz - IN.worldPos );
	vec3 viewDir = normalize ( uCamPosition.xyz - IN.worldPos );
	vec3 halfDir = normalize ( incident + viewDir );
	float dist = length(uLightPosition.xyz - IN.worldPos);
	
	float lambert = max(0.0, dot(incident, IN.worldNormal));
	float rFactor = max(0.0, dot(halfDir, normalize(IN.worldNormal)));
	float sFactor = pow(rFactor , uSpec.w);
	
	vec3 I = normalize(IN.worldPos - uCamPosition.xyz);
    vec3 R = reflect(I, IN.envCoords);
    vec3 reflectColour = texture(uEnvMap, R).rgb;
	
	vec4 colour = vec4(1.0);
	vec3 ambient = colour.rgb * uAmb.rgb;
	vec3 diffuse = colour.rgb * uDiff.rgb * lambert;
	vec3 specular = uSpec.rgb * sFactor;
	vec4 totalColour = vec4(ambient + diffuse + specular, colour.a);
	
	vec3 reflectAmbient = reflectColour * uAmb.rgb;
	vec3 reflectDiffuse = reflectColour * uDiff.rgb * lambert;
	vec4 totalReflect = vec4(reflectAmbient + reflectDiffuse + specular, colour.a);
	
	fragColor = mix(totalReflect, totalColour, 0.0);
}
