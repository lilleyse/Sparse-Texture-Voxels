/******************  GLOBALS  **********************/

#version 420 core
#define POSITION_ATTR 0
#define DEBUG_TRANSFORM_ATTR 1
#define DEBUG_COLOR_ATTR 2
#define PER_FRAME_UBO_BINDING 0
#define VOXEL_TEXTURE_3D_BINDING 0


layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 viewProjection;
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
    uvec2 uResolution;
    float uTime;
};

/***************************************************/

layout(location = POSITION_ATTR) in vec2 position;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}