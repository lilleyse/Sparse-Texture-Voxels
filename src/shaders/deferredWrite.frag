//---------------------------------------------------------
// GLOBALS
//---------------------------------------------------------

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

// should these be vec4's for all of them?
layout (location = DEFERRED_POSITIONS_FBO_BINDING) out vec4 positionOut;
layout (location = DEFERRED_COLORS_FBO_BINDING) out vec4 colorOut;
layout (location = DEFERRED_NORMALS_FBO_BINDING) out vec4 normalOut;

in block
{
    vec3 position;
    vec4 color;
    vec3 normal;

} vertexData;

void main()
{
    positionOut = vec4(vertexData.position, 1.0);
    colorOut = vertexData.color;
    normalOut = vec4(normalize(vertexData.normal), 1.0);

}