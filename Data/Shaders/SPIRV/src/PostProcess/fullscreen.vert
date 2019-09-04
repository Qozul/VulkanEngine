#version 450

//input is between -1 and 1
layout(location = 0) in vec3 iPosition;
layout(location = 0) out vec2 uv;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	uv = iPosition.xy / 2.0 + 0.5;
	gl_Position = vec4(iPosition.xy, 1.0, 1.0);
}
