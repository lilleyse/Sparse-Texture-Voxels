//---------------------------------------------------------
// GLOBALS
//---------------------------------------------------------

#version 420 core

// Vertex attribute indexes
#define POSITION_ATTR            0
#define NORMAL_ATTR              1
#define UV_ATTR                  2
#define PROPERTY_INDEX_ATTR      3
#define DEBUG_TRANSFORM_ATTR     4
#define DEBUG_COLOR_ATTR         5

// Uniform buffer objects binding points
#define PER_FRAME_UBO_BINDING            0
#define LIGHT_UBO_BINDING                1
#define MESH_MATERIAL_ARRAY_BINDING      2
#define POSITION_ARRAY_BINDING           3

// Sampler binding points
#define COLOR_TEXTURE_POSX_3D_BINDING            1 // right direction
#define COLOR_TEXTURE_NEGX_3D_BINDING            2 // left direction
#define COLOR_TEXTURE_POSY_3D_BINDING            3 // up direction
#define COLOR_TEXTURE_NEGY_3D_BINDING            4 // down direction
#define COLOR_TEXTURE_POSZ_3D_BINDING            5 // front direction
#define COLOR_TEXTURE_NEGZ_3D_BINDING            6 // back direction
#define SHADOW_MAP_BINDING                       7
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    8

// Image binding points
#define COLOR_IMAGE_POSX_3D_BINDING              0 // right direction
#define COLOR_IMAGE_NEGX_3D_BINDING              1 // left direction
#define COLOR_IMAGE_POSY_3D_BINDING              2 // up direction
#define COLOR_IMAGE_NEGY_3D_BINDING              3 // down direction
#define COLOR_IMAGE_POSZ_3D_BINDING              4 // front direction
#define COLOR_IMAGE_NEGZ_3D_BINDING              5 // back direction

// Shadow Map FBO
#define SHADOW_MAP_FBO_BINDING     0
#define BLURRED_MAP_FBO_BINDING    1

// Object properties
#define POSITION_INDEX        0
#define MATERIAL_INDEX        1

// Max values
#define MAX_TEXTURE_ARRAYS               10
#define NUM_OBJECTS_MAX                  500
#define NUM_MESHES_MAX                   500
#define MAX_POINT_LIGHTS                 8

layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 uViewProjection;
    mat4 uLightView;
    mat4 uLightProj;
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
    vec3 uLightDir;
    vec3 uLightColor;
    vec2 uResolution;
    float uAspect;
    float uTime;
    float uTimestamp;
    float uFOV;
    float uTextureRes;
    float uNumMips;
    float uSpecularFOV;
    float uSpecularAmount;
    int uCurrentMipLevel;
};

/***************************************************/

layout (location = 0, index = 0) out vec4 fragColor;

in block
{
    vec3 position;
    vec4 color;
    vec3 normal;

} vertexData;

void main()
{
    fragColor = vertexData.color;
}