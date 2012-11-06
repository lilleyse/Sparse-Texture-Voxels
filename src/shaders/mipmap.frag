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

#define TRANSMIT_K  3.0
#define TRANSMIT_MIN 0.05

// alpha blend RGB, average Alpha
vec4 alphaBlend(vec4 front, vec4 back)
{
    front.rgb += (1.0-front.a)*back.rgb;
    //front.a += (1.0-front.a)*back.a;
    front.a = (front.a+back.a)/2.0; // alpha not blended, just averaged
    return front;
}

vec4 calcDirectionalColor(vec4 front1, vec4 front2, vec4 front3, vec4 front4, vec4 back1, vec4 back2, vec4 back3, vec4 back4)
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
    vec4 posXFront1 = texelFetch(tVoxColorTexturePosX, v100, mipLevel);
    vec4 posXFront2 = texelFetch(tVoxColorTexturePosX, v101, mipLevel);
    vec4 posXFront3 = texelFetch(tVoxColorTexturePosX, v110, mipLevel);
    vec4 posXFront4 = texelFetch(tVoxColorTexturePosX, v111, mipLevel);
    vec4 posXBack1  = texelFetch(tVoxColorTexturePosX, v000, mipLevel);
    vec4 posXBack2  = texelFetch(tVoxColorTexturePosX, v001, mipLevel);
    vec4 posXBack3  = texelFetch(tVoxColorTexturePosX, v010, mipLevel);
    vec4 posXBack4  = texelFetch(tVoxColorTexturePosX, v011, mipLevel);

    // negx direction
    vec4 negXFront1 = texelFetch(tVoxColorTextureNegX, v000, mipLevel);
    vec4 negXFront2 = texelFetch(tVoxColorTextureNegX, v001, mipLevel);
    vec4 negXFront3 = texelFetch(tVoxColorTextureNegX, v010, mipLevel);
    vec4 negXFront4 = texelFetch(tVoxColorTextureNegX, v011, mipLevel);
    vec4 negXBack1  = texelFetch(tVoxColorTextureNegX, v100, mipLevel);
    vec4 negXBack2  = texelFetch(tVoxColorTextureNegX, v101, mipLevel);
    vec4 negXBack3  = texelFetch(tVoxColorTextureNegX, v110, mipLevel);
    vec4 negXBack4  = texelFetch(tVoxColorTextureNegX, v111, mipLevel);

    // posy direction
    vec4 posYFront1 = texelFetch(tVoxColorTexturePosY, v010, mipLevel);
    vec4 posYFront2 = texelFetch(tVoxColorTexturePosY, v110, mipLevel);
    vec4 posYFront3 = texelFetch(tVoxColorTexturePosY, v011, mipLevel);
    vec4 posYFront4 = texelFetch(tVoxColorTexturePosY, v111, mipLevel);
    vec4 posYBack1  = texelFetch(tVoxColorTexturePosY, v000, mipLevel);
    vec4 posYBack2  = texelFetch(tVoxColorTexturePosY, v100, mipLevel);
    vec4 posYBack3  = texelFetch(tVoxColorTexturePosY, v001, mipLevel);
    vec4 posYBack4  = texelFetch(tVoxColorTexturePosY, v101, mipLevel);

    // negy direction
    vec4 negYFront1 = texelFetch(tVoxColorTextureNegY, v000, mipLevel);
    vec4 negYFront2 = texelFetch(tVoxColorTextureNegY, v100, mipLevel);
    vec4 negYFront3 = texelFetch(tVoxColorTextureNegY, v001, mipLevel);
    vec4 negYFront4 = texelFetch(tVoxColorTextureNegY, v101, mipLevel);
    vec4 negYBack1  = texelFetch(tVoxColorTextureNegY, v010, mipLevel);
    vec4 negYBack2  = texelFetch(tVoxColorTextureNegY, v110, mipLevel);
    vec4 negYBack3  = texelFetch(tVoxColorTextureNegY, v011, mipLevel);
    vec4 negYBack4  = texelFetch(tVoxColorTextureNegY, v111, mipLevel);

    // posz direction
    vec4 posZFront1 = texelFetch(tVoxColorTexturePosZ, v001, mipLevel);
    vec4 posZFront2 = texelFetch(tVoxColorTexturePosZ, v011, mipLevel);
    vec4 posZFront3 = texelFetch(tVoxColorTexturePosZ, v101, mipLevel);
    vec4 posZFront4 = texelFetch(tVoxColorTexturePosZ, v111, mipLevel);
    vec4 posZBack1  = texelFetch(tVoxColorTexturePosZ, v000, mipLevel);
    vec4 posZBack2  = texelFetch(tVoxColorTexturePosZ, v010, mipLevel);
    vec4 posZBack3  = texelFetch(tVoxColorTexturePosZ, v100, mipLevel);
    vec4 posZBack4  = texelFetch(tVoxColorTexturePosZ, v110, mipLevel);
    
    // negz direction
    vec4 negZFront1 = texelFetch(tVoxColorTextureNegZ, v000, mipLevel);
    vec4 negZFront2 = texelFetch(tVoxColorTextureNegZ, v010, mipLevel);
    vec4 negZFront3 = texelFetch(tVoxColorTextureNegZ, v100, mipLevel);
    vec4 negZFront4 = texelFetch(tVoxColorTextureNegZ, v110, mipLevel);
    vec4 negZBack1  = texelFetch(tVoxColorTextureNegZ, v001, mipLevel);
    vec4 negZBack2  = texelFetch(tVoxColorTextureNegZ, v011, mipLevel);
    vec4 negZBack3  = texelFetch(tVoxColorTextureNegZ, v101, mipLevel);
    vec4 negZBack4  = texelFetch(tVoxColorTextureNegZ, v111, mipLevel);
    
    // calculate the composite color values
    vec4 finalPosX = calcDirectionalColor(posXFront1, posXFront2, posXFront3, posXFront4, posXBack1, posXBack2, posXBack3, posXBack4);
    vec4 finalNegX = calcDirectionalColor(negXFront1, negXFront2, negXFront3, negXFront4, negXBack1, negXBack2, negXBack3, negXBack4);
    vec4 finalPosY = calcDirectionalColor(posYFront1, posYFront2, posYFront3, posYFront4, posYBack1, posYBack2, posYBack3, posYBack4);
    vec4 finalNegY = calcDirectionalColor(negYFront1, negYFront2, negYFront3, negYFront4, negYBack1, negYBack2, negYBack3, negYBack4);
    vec4 finalPosZ = calcDirectionalColor(posZFront1, posZFront2, posZFront3, posZFront4, posZBack1, posZBack2, posZBack3, posZBack4);
    vec4 finalNegZ = calcDirectionalColor(negZFront1, negZFront2, negZFront3, negZFront4, negZBack1, negZBack2, negZBack3, negZBack4);

    // fill the color values
    imageStore(tVoxColorPosX, globalId, finalPosX);
    imageStore(tVoxColorNegX, globalId, finalNegX);
    imageStore(tVoxColorPosY, globalId, finalPosY);
    imageStore(tVoxColorNegY, globalId, finalNegY);
    imageStore(tVoxColorPosZ, globalId, finalPosZ);
    imageStore(tVoxColorNegZ, globalId, finalNegZ);
}