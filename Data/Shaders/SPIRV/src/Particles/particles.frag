#version 450
#extension GL_EXT_nonuniform_qualifier : require

struct PerInstanceParams {
	mat4 model;
	vec4 tint;
};

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout(location = 0) in vec2 inUvCoords;
layout(location = 1) flat in int inInstanceIndex;

layout(location = 0) out vec4 colour;

layout(set = 0, binding = 1) readonly buffer Params {
	PerInstanceParams[] params;
};
layout(set = 0, binding = 2) readonly buffer DIParams {
	uint textureIndices[];
};

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

void main()
{
	colour = texture(texSamplers[nonuniformEXT(textureIndices[SC_MATERIAL_OFFSET + inInstanceIndex])], inUvCoords) + vec4(params[SC_PARAMS_OFFSET + inInstanceIndex].tint.xyz, 0.0);
}
