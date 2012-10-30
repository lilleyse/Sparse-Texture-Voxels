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
#define COLOR_TEXTURE_3D_BINDING                 1
#define NORMAL_TEXTURE_3D_BINDING                2
#define SHADOW_MAP_BINDING                       3
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    4

// Image binding points
#define COLOR_IMAGE_3D_BINDING_BASE              0
#define COLOR_IMAGE_3D_BINDING_CURR              1
#define COLOR_IMAGE_3D_BINDING_NEXT              2
#define NORMAL_IMAGE_3D_BINDING                  3

// Shadow Map FBO
#define SHADOW_MAP_FBO_BINDING      0
#define BLURRED_MAP_FBO_BINDING     1

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
    mat4 uWorldToShadowMap;
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
};


//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = 0) out vec4 fragColor;

layout(binding = COLOR_IMAGE_3D_BINDING_CURR, rgba8) coherent uniform image3D tColorMipCurr;
layout(binding = COLOR_IMAGE_3D_BINDING_NEXT, rgba8) coherent uniform image3D tColorMipNext;

flat in int slice;

// typically use memoryBarrier to do all mipmaps at once, but it's not working right now
void main()
{
    ivec3 globalId = ivec3(ivec2(gl_FragCoord.xy), slice);

    vec4 avgColor = vec4(0.0);
    for(int i = 0; i < 2; i++)
    for(int j = 0; j < 2; j++)
    for(int k = 0; k < 2; k++)
    {
        ivec3 neighbor = globalId*2 + ivec3(i,j,k);
        vec4 neighborColor = imageLoad(tColorMipCurr, neighbor);
        neighborColor.rgb *= neighborColor.a;
        avgColor += neighborColor;
    }
    avgColor.xyz /= avgColor.a;
    avgColor.a /= 8.0;
    //vec4 avgColor = vec4(vec3(globalId)/32.0, 1.0);
    imageStore(tColorMipNext, globalId, avgColor);
}