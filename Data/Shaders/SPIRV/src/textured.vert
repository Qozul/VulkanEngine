#version 450
#extension GL_ARB_separate_shader_objects : enable

struct ElementData {
	mat4 model;
    mat4 mvp;
};

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 worldPos;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(binding = 0) buffer UniformBufferObject {
    ElementData[] uElementData;
} ubo;

void main() {
	gl_Position = ubo.uElementData[gl_InstanceIndex].mvp * vec4(iPosition, 1.0);
	texUV = iTextureCoord;
	worldPos = (ubo.uElementData[gl_InstanceIndex].model * vec4(iPosition, 1.0)).xyz;
	normal = mat3(transpose(inverse(ubo.uElementData[gl_InstanceIndex].model))) * iNormal;
}
