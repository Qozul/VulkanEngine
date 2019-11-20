#version 450
#extension GL_EXT_nonuniform_qualifier : require

struct Material {
	uint colourIdx;
};

layout (constant_id = 0) const float SC_NEAR_Z = 0.1;
layout (constant_id = 1) const float SC_FAR_Z = 1000.0;
layout (constant_id = 2) const uint SC_MATERIAL_OFFSET = 0;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

layout(set = 0, binding = 2) readonly buffer DescriptorIndexBuffer
{
	Material materialData[];
};

layout(set = 1, binding = 2) uniform sampler2D texSamplers[];

float linearizeDepth(float depth)
{
  return (2.0 * SC_NEAR_Z) / (SC_FAR_Z + SC_NEAR_Z - depth * (SC_FAR_Z - SC_NEAR_Z));
}

void main()
{
	Material material = materialData[SC_MATERIAL_OFFSET];
	colour = texture(texSamplers[nonuniformEXT(material.colourIdx)], uv);
}
