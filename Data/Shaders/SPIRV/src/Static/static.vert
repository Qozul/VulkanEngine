#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Material {
    mat4 model;
	vec4 diffuseColour;
	vec4 specularColour;
};
struct ElementData {
    mat4 mvp;
};

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

layout(constant_id = 0) const uint SC_MVP_OFFSET = 0;
layout(constant_id = 1) const uint SC_PARAMS_OFFSET = 0;

layout (location = 0) in vec3 iPosition;
layout (location = 1) in vec2 iTextureCoord;
layout (location = 2) in vec3 iNormal;

layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 worldPos;
layout (location = 3) flat out int instanceIndex;
layout (location = 4) out vec4 shadowCoord;
layout (location = 5) flat out uint shadowMapIdx;

layout(push_constant) uniform PushConstants {
	mat4 shadowMatrix;
	vec4 cameraPosition;
	vec3 mainLightPosition;
	uint shadowTextureIdx;
} PC;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 1) readonly buffer MaterialData
{
	Material[] materials;
};

layout(set = 0, binding = 0) readonly buffer StorageBuffer {
    ElementData[] uElementData;
} buf;

void main() {
	instanceIndex = gl_InstanceIndex;
	gl_Position = buf.uElementData[SC_MVP_OFFSET + gl_InstanceIndex].mvp * vec4(iPosition, 1.0);
	texUV = iTextureCoord;
	worldPos = (materials[SC_PARAMS_OFFSET + gl_InstanceIndex].model * vec4(iPosition, 1.0)).xyz;
	normal = mat3(transpose(inverse(materials[SC_PARAMS_OFFSET + gl_InstanceIndex].model))) * iNormal;

	shadowCoord = (biasMat * PC.shadowMatrix * materials[SC_PARAMS_OFFSET + gl_InstanceIndex].model) * vec4(iPosition, 1.0);
	shadowMapIdx = PC.shadowTextureIdx;
}
