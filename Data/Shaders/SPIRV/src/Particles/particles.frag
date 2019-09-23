#version 450

layout(location = 0) in vec2 inUvCoords;
layout(location = 1) flat in int inInstanceIndex;

layout(location = 0) out vec4 colour;

layout (binding = 0) uniform Params {
	mat4 model;
	mat4 mvp;
	vec4 tint;
} UBO;

layout(binding = 1) uniform sampler2D tex;

void main()
{
	//colour = texture(tex, inUvCoords);// + PC.tint;
	colour = vec4(1.0, 0.0, 0.0, 1.0);//PC.tint;
}
