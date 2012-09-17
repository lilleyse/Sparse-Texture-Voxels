#version 420 core
#define POSITION_ATTR 0
#define DEBUG_TRANSFORM_ATTR 1
#define DEBUG_COLOR_ATTR 2
#define PER_FRAME_UBO_BINDING 0

layout(location = POSITION_ATTR) in vec3 position;
layout(location = DEBUG_TRANSFORM_ATTR) in vec4 transformation;
layout(location = DEBUG_COLOR_ATTR) in vec4 color;

layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
	mat4 viewProjection;
};

out block
{
	flat vec4 color;
} vertexData;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	// Create the model matrix
	float scale = transformation.w;
	mat4 modelMatrix = mat4(scale);
	modelMatrix[3] = vec4(transformation.xyz, 1.0);

	// Caluclate the clip space position
	gl_Position = viewProjection * modelMatrix * vec4(position, 1.0);
	
	vertexData.color = color;
}