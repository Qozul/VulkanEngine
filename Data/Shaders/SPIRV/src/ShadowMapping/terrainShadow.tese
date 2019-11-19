#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) flat in uint heightmapIdx[];

layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

void main() {
	
}