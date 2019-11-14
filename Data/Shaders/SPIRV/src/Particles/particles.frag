#version 450
#extension GL_EXT_nonuniform_qualifier : require

struct PerInstanceParams {
	mat4 model;
	mat4 mvp;
	vec4 tint;
};

layout(location = 0) in vec2 inUvCoords;
layout(location = 1) flat in int inInstanceIndex;

layout(location = 0) out vec4 colour;

layout(constant_id = 0) const int SC_MAX_PARTICLE_SYSTEMS = 111;

layout(set = 0, binding = 0) uniform Params {
	PerInstanceParams[SC_MAX_PARTICLE_SYSTEMS] params;
} UBO;
layout(set = 0, binding = 1) buffer DIParams {
	 uint textureIndices[];
};

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

void main()
{
	colour = texture(texSamplers[nonuniformEXT(textureIndices[inInstanceIndex])], inUvCoords) + UBO.params[inInstanceIndex].tint;
}
