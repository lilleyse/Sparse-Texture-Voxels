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
    float uAspect;
    float uTime;
    float uFOV;
};

/***************************************************/

layout(location = POSITION_ATTR) in vec2 position;

out gl_PerVertex
{
    vec4 gl_Position;
};

out vec2 vUV;

void main()
{
    vUV = (position+1.0)/2.0;
    vUV.y = 1.0-vUV.y;

    gl_Position = vec4(position, 0.0, 1.0);
}