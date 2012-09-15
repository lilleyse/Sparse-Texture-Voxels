#version 420 core
#extension GL_AMD_sparse_texture : enable
#define POSITION 0

layout(location = POSITION) in vec2 position;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position = vec4(position, 0.0, 1.0);
}