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
#define NOISE_TEXTURE_2D_BINDING                 4
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    5

// Image binding points
#define COLOR_IMAGE_3D_BINDING_BASE              0
#define COLOR_IMAGE_3D_BINDING_CURR              1
#define COLOR_IMAGE_3D_BINDING_NEXT              2
#define NORMAL_IMAGE_3D_BINDING_BASE             3
#define NORMAL_IMAGE_3D_BINDING_CURR             4
#define NORMAL_IMAGE_3D_BINDING_NEXT             5

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
};

layout(binding = SHADOW_MAP_BINDING) uniform sampler2D shadowMap;  
layout(binding = DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];
layout(binding = COLOR_IMAGE_3D_BINDING_BASE, rgba8) writeonly uniform image3D tVoxColor;
layout(binding = NORMAL_IMAGE_3D_BINDING_BASE, rgba8_snorm) writeonly uniform image3D tVoxNormal;

in block
{
    vec3 position;
    vec3 normal;
    vec3 shadowMapPos;
    vec2 uv;
    flat ivec2 propertyIndex;
} vertexData;

struct MeshMaterial
{
    vec4 diffuseColor;
    vec4 specularColor;
    ivec2 textureLayer;
};

layout(std140, binding = MESH_MATERIAL_ARRAY_BINDING) uniform MeshMaterialArray
{
    MeshMaterial meshMaterialArray[NUM_MESHES_MAX];
};


MeshMaterial getMeshMaterial()
{
    int index = vertexData.propertyIndex[MATERIAL_INDEX];
    return meshMaterialArray[index];
}

vec4 getDiffuseColor(MeshMaterial material)
{
    int textureId = material.textureLayer.x;
    int textureLayer = material.textureLayer.y;
    vec4 diffuseColor = textureId == -1 ? material.diffuseColor : texture(diffuseTextures[textureId], vec3(vertexData.uv, textureLayer));
    return diffuseColor;
}

float getVisibility()
{
	float fragLightDepth = vertexData.shadowMapPos.z;
    float shadowMapDepth = texture(shadowMap, vertexData.shadowMapPos.xy).r;

    // Less darknessFactor means lighter shadows
    float darknessFactor = 50.0;
    return clamp(exp(darknessFactor * (shadowMapDepth - fragLightDepth)), 0.0, 1.0);
}

void main()
{
    float visibility = getVisibility();
    vec4 diffuse = getDiffuseColor(getMeshMaterial());
    vec3 normal = normalize(vertexData.normal);
    vec3 position = vertexData.position;
    float LdotN = pow(max(dot(uLightDir, normal), 0.0), 0.5);
    vec4 outColor = vec4(diffuse.rgb*uLightColor*visibility*LdotN, diffuse.a);
    //in the future, the magnitude of reflectedDirection will be the specularity 
    vec3 reflectedDirection = reflect(-uLightDir, normal);
    vec4 outNormal = vec4(reflectedDirection, 1.0);

    // write to image
    ivec3 voxelPos = ivec3(vertexData.position*float(uResolution.x));
    imageStore(tVoxColor, voxelPos, outColor);
    imageStore(tVoxNormal, voxelPos, outNormal);
}
