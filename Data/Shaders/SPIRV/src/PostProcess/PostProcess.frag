#version 450

layout (constant_id = 0) const float SC_NEAR_Z = 0.1;
layout (constant_id = 1) const float SC_FAR_Z = 10.0;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

layout(binding = 0) uniform sampler3D apScatteringTexture;
layout(binding = 1) uniform sampler2D apTransmittanceTexture;
layout(binding = 2) uniform sampler2D geometryColourTexture;
layout(binding = 3) uniform sampler2D geometryDepthTexture;

float linearizeDepth(float depth)
{
  return (2.0 * SC_NEAR_Z) / (SC_FAR_Z + SC_NEAR_Z - depth * (SC_FAR_Z - SC_NEAR_Z));
}

void main()
{
	//vec4 sCol = texture(apScatteringTexture, vec3(uv, depth));
	//vec4 tCol = texture(apTransmittanceTexture, vec3(uv, depth));
	
	float depth = texture(geometryDepthTexture, uv).r;
	depth = clamp(linearizeDepth(depth), 0.0, 1.0);
	
	/*vec3 V = getNearPlaneWorldPosition(uv, inverseViewProj);
	vec3 L = sunDir;
	
	float ctheta = dot(V, L);
	ctheta.
	
	vec4 Fex = exp(-(atmosphere.betaRay + atmosphere.betaMie) * depth);
	vec4 Lin = (rayleightPhase(theta*/
	
	colour = texture(geometryColourTexture, uv);
	/*if (depth < 1.0) {
		vec4 mixColour = texture(apTransmittanceTexture, uv) * depth;
		colour = mix(colour, mixColour, 0.5);
	}*/
}
