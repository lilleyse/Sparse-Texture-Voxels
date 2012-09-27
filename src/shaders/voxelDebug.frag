/******************  GLOBALS  **********************/

#version 420 core
#define POSITION_ATTR 0
#define DEBUG_TRANSFORM_ATTR 1
#define DEBUG_COLOR_ATTR 2
#define PER_FRAME_UBO_BINDING 0
#define COLOR_TEXTURE_3D_BINDING 0
#define NORMAL_TEXTURE_3D_BINDING 1

layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 viewProjection;
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
    vec2 uResolution;
    float uTime;
    float uFOV;
};

/***************************************************/

layout (location = 0, index = 0) out vec4 fragColor;

in block
{
    flat vec4 color;
} vertexData;

void main()
{
    fragColor = vertexData.color;
}