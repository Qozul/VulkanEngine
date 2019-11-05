// See https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
// for reference on this technique.
#version 450

layout(location = 0) out vec2 uv;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
}
