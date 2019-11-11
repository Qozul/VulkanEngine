#version 450

struct ElementData {
	mat4 model;
    mat4 mvp;
};

layout(quads, equal_spacing, cw) in;

layout (location = 0) in vec2 iTexUV[];

layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec3 normal;

layout(set = 0, binding = 0) readonly buffer UniformBufferObject {
    ElementData uElementData;
} ubo;

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

layout(set = 2, binding = 0) uniform sampler2D heightmap;
layout(set = 2, binding = 2) uniform sampler2D normalmap;

const float maxHeight = 100.0;

float bilinearFilter(vec4 gathered)
{
	float g0 = mix(gathered.x, gathered.y, 0.5);
	float g1 = mix(gathered.z, gathered.w, 0.5);
	return mix(g0, g1, 0.5);
}

void main(void)
{
	vec2 uv1 = mix(iTexUV[0], iTexUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(iTexUV[3], iTexUV[2], gl_TessCoord.x);
	texUV = mix(uv1, uv2, gl_TessCoord.y);
	
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	position.y -= bilinearFilter(texture(heightmap, texUV)) * maxHeight;
	
	gl_Position = ubo.uElementData.mvp * position;
	worldPos = (ubo.uElementData.model * position).xyz;
	
	//normal = vec3(
	//	bilinearFilter(textureGather(normalmap, texUV, 1)), 
	//	bilinearFilter(textureGather(normalmap, texUV, 2)), 
	//	bilinearFilter(textureGather(normalmap, texUV, 0))
	//);
	normal = texture(normalmap, texUV).rgb;
	float tmp = normal.r;
	normal.r = normal.g;
	normal.g = normal.b;
	normal.b = tmp;
}
