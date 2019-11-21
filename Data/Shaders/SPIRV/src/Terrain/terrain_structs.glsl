struct Params {
	mat4 model;
	vec4 heights;
	float distanceFarMinusClose;
	float closeDistance;
	float patchRadius;
	float maxTessellationWeight;
	vec4 frustumPlanes[6];
};
struct TextureIndices {
	uint normalmapIdx;
	uint albedoIdx0;
	uint albedoIdx1;
	uint albedoIdx2;
};
