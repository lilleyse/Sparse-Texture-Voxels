/******************  GLOBALS  **********************/

#version 420 core
#define POSITION_ATTR 0
#define NORMAL_ATTR 1
#define DEBUG_TRANSFORM_ATTR 2
#define DEBUG_COLOR_ATTR 3
#define PER_FRAME_UBO_BINDING 0
#define COLOR_TEXTURE_3D_BINDING 0
#define NORMAL_TEXTURE_3D_BINDING 1
#define DEFERRED_POSITIONS_TEXTURE_BINDING 2
#define DEFERRED_COLORS_TEXTURE_BINDING 3
#define DEFERRED_NORMALS_TEXTURE_BINDING 4
#define DEFERRED_POSITIONS_FBO_BINDING 0
#define DEFERRED_COLORS_FBO_BINDING 1
#define DEFERRED_NORMALS_FBO_BINDING 2

layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 viewProjection;
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
    vec2 uResolution;
    float uAspect;
    float uTime;
    float uFOV;
};

/***************************************************/

layout(location = POSITION_ATTR) in vec3 position;
layout(location = NORMAL_ATTR) in vec3 normal;
layout(location = DEBUG_TRANSFORM_ATTR) in vec4 transformation;
layout(location = DEBUG_COLOR_ATTR) in vec4 color;

out block
{
    vec3 position;
    vec4 color;
    vec3 normal;

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

    vec4 worldPosition = modelMatrix * vec4(position, 1.0);

    // Caluclate the clip space position
    gl_Position = viewProjection * worldPosition;
    
    vertexData.position = vec3(worldPosition);
    vertexData.color = color;
    vertexData.normal = normal;

}