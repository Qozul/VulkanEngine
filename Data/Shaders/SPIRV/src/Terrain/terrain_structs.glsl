struct Params {
	mat4 model;
	vec4 diffuseColour;
	vec4 specularColour;
	float distanceFarMinusClose;
	float closeDistance;
	float patchRadius;
	float maxTessellationWeight ;
	vec4 frustumPlanes[6];
};
struct TextureIndices {
	uint heightmapIdx;
	uint normalmapIdx;
	uint diffuseIdx;
};
