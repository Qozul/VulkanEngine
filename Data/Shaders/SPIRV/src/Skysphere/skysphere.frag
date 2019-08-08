#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Material {
};

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) buffer MaterialData
{
	Material material;
};

void main() {

	fragColor = vec4(1.0);
}
