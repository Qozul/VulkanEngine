#version 450
#extension GL_EXT_nonuniform_qualifier : require

#define NUM_VERTS 4

layout(vertices = NUM_VERTS) out;

layout(location = 0) flat in uint heightmapIdx[];
layout(location = 0) flat out uint outHeightmapIdx[];

void main() {

}