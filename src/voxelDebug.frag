#version 420 core

layout (location = 0, index = 0) out vec4 fragColor;


in block
{
	flat vec4 color;
} vertexData;

void main()
{
	fragColor = vertexData.color;
}