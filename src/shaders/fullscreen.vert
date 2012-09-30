//---------------------------------------------------------
// GLOBALS
//---------------------------------------------------------

#version 420 core
#define POSITION_ATTR 0
#define NORMAL_ATTR 1
#define DEBUG_TRANSFORM_ATTR 2
#define DEBUG_COLOR_ATTR 3
#define PER_FRAME_UBO_BINDING 0
#define DEFERRED_POSITIONS_BINDING 0
#define DEFERRED_COLORS_BINDING 1
#define DEFERRED_NORMALS_BINDING 2
#define COLOR_TEXTURE_3D_BINDING 3
#define NORMAL_TEXTURE_3D_BINDING 4

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


//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = POSITION_ATTR) in vec2 position;

out gl_PerVertex
{
    vec4 gl_Position;
};

out vec2 vUV;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    vUV = (position+1.0)/2.0;

    gl_Position = vec4(position, 0.0, 1.0);
}