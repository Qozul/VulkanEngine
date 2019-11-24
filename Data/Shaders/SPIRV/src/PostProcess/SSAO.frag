#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout (constant_id = 0) const uint SC_GEOMETRY_DEPTH_IDX = 0;
layout (constant_id = 1) const uint SC_G_BUFFER_NORMALS_IDX = 0;
layout (constant_id = 2) const uint SC_NOISE_TEXTURE_IDX = 0;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;
layout(location = 2) out vec4 outColour;

layout(set = GLOBAL_SET, binding = CAMERA_INFO_BINDING) uniform CameraInfo {
	mat4 inverseViewProj;
	mat4 viewMatrix;
	mat4 projMatrix;
	vec3 position;
	float nearPlaneZ;
	float farPlaneZ;
	float screenX;
	float screenY;
} Camera;

layout(set = GLOBAL_SET, binding = POST_PROCESS_BINDING) uniform PostProcessInfo {
	vec2 ssaoNoiseScale;
	int ssaoKernelSize;
	float ssaoRadius;
	vec4 ssaoSamples[64];
	float ssaoBias;
} Post;

vec3 sampleDepth(vec2 coords) {
	vec4 clip = inverse(Camera.projMatrix) * vec4(vec3(coords * 2.0 - 1.0, texture(texSamplers[nonuniformEXT(SC_GEOMETRY_DEPTH_IDX)], coords).r * 2.0 - 1.0), 0.0);
	vec3 position = clip.xyz / clip.w;
	return position;
}

void main()
{
	vec3 normal = normalize(texture(texSamplers[nonuniformEXT(SC_G_BUFFER_NORMALS_IDX)], uv).xyz * 2.0 - 1.0); // world space
	vec3 random = normalize(texture(texSamplers[nonuniformEXT(SC_NOISE_TEXTURE_IDX)], uv * Post.ssaoNoiseScale).xyz);
	
	vec3 position = sampleDepth(uv);
	
	normal = transpose(inverse(mat3(Camera.viewMatrix))) * normal; // View space
	mat3 TBN = calculateTBN(normal, normalize(random - normal * dot(random, normal)));
	
	float occlusion = 0.0;
	for (int i = 0; i < Post.ssaoKernelSize; ++i) {
		vec3 samp = TBN * Post.ssaoSamples[i].xyz;
		samp = position + samp * Post.ssaoRadius;
		
		vec4 off = vec4(samp, 1.0);
		off = Camera.projMatrix * off;
		off.xyz /= off.w;
		off.xyz = off.xyz * 0.5 + 0.5;
		
		float depth = sampleDepth(off.xy).z;
		float rangeCheck = smoothstep(0.0, 1.0, Post.ssaoRadius / abs(position.z - depth));
		occlusion += (depth >= samp.z + Post.ssaoBias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / float(Post.ssaoKernelSize));
	outColour = vec4(occlusion, occlusion, occlusion, 1.0);
}
