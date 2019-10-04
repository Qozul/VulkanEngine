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
	float depth = texture(geometryDepthTexture, uv).r;
	depth = clamp(linearizeDepth(depth), 0.0, 1.0);
	
	colour = texture(geometryColourTexture, uv);
}
