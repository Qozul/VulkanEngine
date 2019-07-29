#version 440 core

layout(triangles, equal_spacing, ccw) in;

uniform mat4 uModelMat 	= mat4(1.0f);

uniform float uSpiralTheta = 0.0; // angle of rotation of spiral
uniform float uNumSpirals = 0.0;
uniform float uPoleOffset = 0.0;

const vec3 kPoleLeft = vec3(1.0, 0.0, 0.0);
const vec3 kPoleRight = vec3(-1.0, 0.0, 0.0);
const vec3 kLocalOrigin = vec3(0.0);

in Vertex {
	vec2 texUV;
} IN[];

out Vertex {
	vec3 idxPos;
	vec2 texUV;
} OUT;


void VertexSpiral(inout vec3 v)
{
	// Apply a spiral function, as if spiraling around a black hole
	float dist = distance(v, kLocalOrigin);
	v.x -= uNumSpirals * dist*cos(dist*uSpiralTheta);
	v.z -= uNumSpirals  * dist*sin(dist*uSpiralTheta);
}

void ConsumeVertex(inout vec3 v, vec3 pole)
{
	float dist = distance(v, pole);
	if (dist <= uPoleOffset)
		v = vec3(0.0);
}

void main(void)
{
	vec3 p0 = gl_TessCoord.x * gl_in[0].gl_Position.xyz;
	vec3 p1 = gl_TessCoord.y * gl_in[1].gl_Position.xyz;
	vec3 p2 = gl_TessCoord.z * gl_in[2].gl_Position.xyz;
	//vec3 position = VertexMorph(p0) + VertexMorph(p1) + VertexMorph(p2);
	
	vec3 position = p0 + p1 + p2;
	OUT.idxPos = position;
	// Morph around a spiral and have the black hole "consume" the cube from both ends
	VertexSpiral(position);
	ConsumeVertex(position, kPoleLeft);
	ConsumeVertex(position, kPoleRight);
	gl_Position = vec4(position, 1.0);
	
	// This will always be the same because TCS doesn't vary
	OUT.texUV = gl_TessCoord.x * IN[0].texUV + gl_TessCoord.y * IN[1].texUV + gl_TessCoord.z * IN[2].texUV;
	
}
