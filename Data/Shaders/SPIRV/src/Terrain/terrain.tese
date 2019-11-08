#version 450

struct ElementData {
	mat4 model;
    mat4 mvp;
};

layout(quads, equal_spacing, cw) in;

layout (location = 0) in vec2 iTexUV[];

layout (location = 0) out vec2 texUV;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 worldPos;

layout(set = 0, binding = 0) readonly buffer UniformBufferObject {
    ElementData uElementData;
} ubo;

layout(set = 2, binding = 0) uniform sampler2D heightmap;

const float maxHeight = 100.0;

vec3 generateNormal(in vec3 worldPosition)
{
	vec3 normal = vec3(0.0, 1.0, 0.0);

	// Do the thing derivatives in fragment shader
	

	return normal;
}

void main(void)
{
	vec2 uv1 = mix(iTexUV[0], iTexUV[1], gl_TessCoord.x);
	vec2 uv2 = mix(iTexUV[3], iTexUV[2], gl_TessCoord.x);
	texUV = mix(uv1, uv2, gl_TessCoord.y);
	
	vec4 pos1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 pos2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 position = mix(pos1, pos2, gl_TessCoord.y);
	position.y -= textureLod(heightmap, texUV, 0.0).r * maxHeight;
	
	gl_Position = ubo.uElementData.mvp * position;
	worldPos = (ubo.uElementData.model * position).xyz;
	
	normal = generateNormal(worldPos);
}
