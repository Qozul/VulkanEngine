#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Material {
	vec4 diffuseColour;
	vec4 specularColour;
    mat4 model;
};
struct ElementData {
    mat4 mvp;
};

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 worldPos;
layout (location = 3) flat out int instanceIndex;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 1, binding = 1) readonly buffer MaterialData
{
	Material materials[];
};

layout(set = 1, binding = 0) readonly buffer StorageBuffer {
    ElementData[] uElementData;
} buf;

void main() {
	instanceIndex = gl_InstanceIndex;
	gl_Position = buf.uElementData[1 + gl_InstanceIndex].mvp * vec4(iPosition, 1.0);
	texUV = iTextureCoord;
	worldPos = (materials[1 + gl_InstanceIndex].model * vec4(iPosition, 1.0)).xyz;
	normal = mat3(transpose(inverse(materials[gl_InstanceIndex].model))) * iNormal;
}
