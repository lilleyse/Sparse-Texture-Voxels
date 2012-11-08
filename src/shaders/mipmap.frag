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


//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = 0) out vec4 fragColor;

layout(binding = COLOR_TEXTURE_POSX_3D_BINDING) uniform sampler3D tVoxColorTexturePosX;
layout(binding = COLOR_TEXTURE_NEGX_3D_BINDING) uniform sampler3D tVoxColorTextureNegX;
layout(binding = COLOR_TEXTURE_POSY_3D_BINDING) uniform sampler3D tVoxColorTexturePosY;
layout(binding = COLOR_TEXTURE_NEGY_3D_BINDING) uniform sampler3D tVoxColorTextureNegY;
layout(binding = COLOR_TEXTURE_POSZ_3D_BINDING) uniform sampler3D tVoxColorTexturePosZ;
layout(binding = COLOR_TEXTURE_NEGZ_3D_BINDING) uniform sampler3D tVoxColorTextureNegZ;

layout(binding = COLOR_IMAGE_POSX_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosX;
layout(binding = COLOR_IMAGE_NEGX_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegX;
layout(binding = COLOR_IMAGE_POSY_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosY;
layout(binding = COLOR_IMAGE_NEGY_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegY;
layout(binding = COLOR_IMAGE_POSZ_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosZ;
layout(binding = COLOR_IMAGE_NEGZ_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegZ;

flat in int slice;

// alpha blend RGB, average Alpha
vec4 alphaBlend(vec4 front, vec4 back)
{
    front.rgb += (1.0-front.a)*back.rgb;
    //front.a += (1.0-front.a)*back.a;
    front.a = (front.a+back.a)/2.0; // alpha not blended, just averaged
    return front;
}

vec4 calcDirectionalColor(
    vec4 front1, vec4 front2, vec4 front3, vec4 front4, 
    vec4 back1, vec4 back2, vec4 back3, vec4 back4)
{
    vec4 color1 = alphaBlend(front1, back1);
    vec4 color2 = alphaBlend(front2, back2);
    vec4 color3 = alphaBlend(front3, back3);
    vec4 color4 = alphaBlend(front4, back4);
    color1.rgb *= color1.a;
    color2.rgb *= color2.a;
    color3.rgb *= color3.a;
    color4.rgb *= color4.a;
    vec4 color = color1 + color2 + color3 + color4;
    color.rgb /= color.a;
    color.a /= 4.0;
    return color;
}

void main()
{
    int mipLevel = uCurrentMipLevel-1;
    ivec3 globalId = ivec3(ivec2(gl_FragCoord.xy), slice);
    ivec3 oldGlobalId = globalId*2;

    ivec3 v000 = oldGlobalId + ivec3(0,0,0);
    ivec3 v100 = oldGlobalId + ivec3(1,0,0);
    ivec3 v010 = oldGlobalId + ivec3(0,1,0);
    ivec3 v001 = oldGlobalId + ivec3(0,0,1);
    ivec3 v110 = oldGlobalId + ivec3(1,1,0);
    ivec3 v011 = oldGlobalId + ivec3(0,1,1);
    ivec3 v101 = oldGlobalId + ivec3(1,0,1);
    ivec3 v111 = oldGlobalId + ivec3(1,1,1);

    // posx direction
    vec4 finalPosX = calcDirectionalColor(
        texelFetch(tVoxColorTexturePosX, v100, mipLevel),
        texelFetch(tVoxColorTexturePosX, v101, mipLevel),
        texelFetch(tVoxColorTexturePosX, v110, mipLevel),
        texelFetch(tVoxColorTexturePosX, v111, mipLevel),
        texelFetch(tVoxColorTexturePosX, v000, mipLevel),
        texelFetch(tVoxColorTexturePosX, v001, mipLevel),
        texelFetch(tVoxColorTexturePosX, v010, mipLevel),
        texelFetch(tVoxColorTexturePosX, v011, mipLevel)
    );

    // negx direction
    vec4 finalNegX = calcDirectionalColor(
        texelFetch(tVoxColorTextureNegX, v000, mipLevel),
        texelFetch(tVoxColorTextureNegX, v001, mipLevel),
        texelFetch(tVoxColorTextureNegX, v010, mipLevel),
        texelFetch(tVoxColorTextureNegX, v011, mipLevel),
        texelFetch(tVoxColorTextureNegX, v100, mipLevel),
        texelFetch(tVoxColorTextureNegX, v101, mipLevel),
        texelFetch(tVoxColorTextureNegX, v110, mipLevel),
        texelFetch(tVoxColorTextureNegX, v111, mipLevel)
    );

    // posy direction
    vec4 finalPosY = calcDirectionalColor(
        texelFetch(tVoxColorTexturePosY, v010, mipLevel),
        texelFetch(tVoxColorTexturePosY, v110, mipLevel),
        texelFetch(tVoxColorTexturePosY, v011, mipLevel),
        texelFetch(tVoxColorTexturePosY, v111, mipLevel),
        texelFetch(tVoxColorTexturePosY, v000, mipLevel),
        texelFetch(tVoxColorTexturePosY, v100, mipLevel),
        texelFetch(tVoxColorTexturePosY, v001, mipLevel),
        texelFetch(tVoxColorTexturePosY, v101, mipLevel)
    );

    // negy direction
    vec4 finalNegY = calcDirectionalColor(
        texelFetch(tVoxColorTextureNegY, v000, mipLevel),
        texelFetch(tVoxColorTextureNegY, v100, mipLevel),
        texelFetch(tVoxColorTextureNegY, v001, mipLevel),
        texelFetch(tVoxColorTextureNegY, v101, mipLevel),
        texelFetch(tVoxColorTextureNegY, v010, mipLevel),
        texelFetch(tVoxColorTextureNegY, v110, mipLevel),
        texelFetch(tVoxColorTextureNegY, v011, mipLevel),
        texelFetch(tVoxColorTextureNegY, v111, mipLevel)
    );

    // posz direction
    vec4 finalPosZ = calcDirectionalColor(
        texelFetch(tVoxColorTexturePosZ, v001, mipLevel),
        texelFetch(tVoxColorTexturePosZ, v011, mipLevel),
        texelFetch(tVoxColorTexturePosZ, v101, mipLevel),
        texelFetch(tVoxColorTexturePosZ, v111, mipLevel),
        texelFetch(tVoxColorTexturePosZ, v000, mipLevel),
        texelFetch(tVoxColorTexturePosZ, v010, mipLevel),
        texelFetch(tVoxColorTexturePosZ, v100, mipLevel),
        texelFetch(tVoxColorTexturePosZ, v110, mipLevel)
    );
    
    // negz direction
    vec4 finalNegZ = calcDirectionalColor(
        texelFetch(tVoxColorTextureNegZ, v000, mipLevel),
        texelFetch(tVoxColorTextureNegZ, v010, mipLevel),
        texelFetch(tVoxColorTextureNegZ, v100, mipLevel),
        texelFetch(tVoxColorTextureNegZ, v110, mipLevel),
        texelFetch(tVoxColorTextureNegZ, v001, mipLevel),
        texelFetch(tVoxColorTextureNegZ, v011, mipLevel),
        texelFetch(tVoxColorTextureNegZ, v101, mipLevel),
        texelFetch(tVoxColorTextureNegZ, v111, mipLevel)
    );

    // fill the color values
    imageStore(tVoxColorPosX, globalId, finalPosX);
    imageStore(tVoxColorNegX, globalId, finalNegX);
    imageStore(tVoxColorPosY, globalId, finalPosY);
    imageStore(tVoxColorNegY, globalId, finalNegY);
    imageStore(tVoxColorPosZ, globalId, finalPosZ);
    imageStore(tVoxColorNegZ, globalId, finalNegZ);
}