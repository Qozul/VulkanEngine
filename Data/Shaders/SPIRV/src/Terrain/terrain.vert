#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#define USE_VERTEX_PUSH_CONSTANTS
#include "../common.glsl"
#include "terrain_structs.glsl"

layout(constant_id = 0) const uint SC_PARAMS_OFFSET = 0;

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTextureCoord;
layout(location = 2) in vec3 iNormal;

layout (location = 0) out vec2 texUV;
layout (location = 1) flat out int instanceIndex;
layout (location = 2) out vec4 shadowCoord;
layout (location = 3) flat out uint shadowMapIdx;
layout (location = 4) out vec3 normal;
layout (location = 5) flat out vec3 outCamPos;

layout(set = COMMON_SET, binding = COMMON_PARAMS_BINDING) readonly buffer MaterialData
{
	Params materials[];
};

void main() {
	instanceIndex = gl_InstanceIndex;
	gl_Position = vec4(iPosition, 1.0);
	texUV = iTextureCoord;
	normal = iNormal;
	shadowCoord = (BIAS_MATRIX * PC.shadowMatrix * materials[SC_PARAMS_OFFSET + instanceIndex].model) * vec4(iPosition, 1.0);
	shadowMapIdx = PC.shadowTextureIdx;
	outCamPos = PC.cameraPosition.xyz;
}
