#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"
#include "terrain_structs.glsl"

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout (location = 0) out vec2 texUV;
layout (location = 1) flat out int instanceIndex;
layout (location = 2) out vec4 shadowCoord;
layout (location = 3) flat out uint shadowMapIdx;
layout (location = 4) out vec3 normal;

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer MaterialData
{
	Params materials[];
};

layout(push_constant) uniform PushConstants {
	mat4 shadowMatrix;
	vec4 cameraPosition;
	vec3 mainLightPosition;
	uint shadowTextureIdx;
} PC;

void main() {
	instanceIndex = gl_InstanceIndex;
	gl_Position = vec4(iPosition, 1.0);
	texUV = iTextureCoord;
	normal = iNormal;
	shadowCoord = (biasMat * PC.shadowMatrix * materials[SC_PARAMS_OFFSET + instanceIndex].model) * vec4(iPosition, 1.0);
	shadowMapIdx = PC.shadowTextureIdx;
}
