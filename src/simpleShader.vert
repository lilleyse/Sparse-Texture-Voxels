#version 420 core
#define POSITION_ATTRIBUTE 0

layout(location = POSITION_ATTRIBUTE) in vec2 position;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position = vec4(position, 0.0, 1.0);
}