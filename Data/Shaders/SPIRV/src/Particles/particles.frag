#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

struct PerInstanceParams {
	mat4 model;
	vec4 tint;
};

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;
layout(constant_id = 1) const uint SC_MATERIAL_OFFSET = 0;

layout(location = 0) in vec2 inUvCoords;
layout(location = 1) flat in int inInstanceIndex;

layout(location = 0) out vec4 colour;

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer Params {
	PerInstanceParams[] params;
};
layout(set = COMMON_SET, binding = COMMON_MATERIALS_BINDING) readonly buffer DIParams {
	uint textureIndices[];
};

void main()
{
	colour = texture(texSamplers[nonuniformEXT(textureIndices[SC_MATERIAL_OFFSET + inInstanceIndex])], inUvCoords);
}
