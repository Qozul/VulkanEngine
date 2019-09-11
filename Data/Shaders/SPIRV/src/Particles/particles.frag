#version 450

layout(location = 0) in vec2 inUvCoords;

layout(location = 0) out vec4 colour;

layout(binding = 0) uniform sampler2D tex;

layout(push_constant) uniform PushConstants {
	layout(offset = 80) vec4 tint;
} PC;

void main()
{
	colour = texture(tex, inUvCoords);// + PC.tint;
}
