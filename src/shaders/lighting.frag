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
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    3

// Image binding points
#define COLOR_IMAGE_3D_BINDING_BASE              0
#define COLOR_IMAGE_3D_BINDING_CURR              1
#define COLOR_IMAGE_3D_BINDING_NEXT              2
#define NORMAL_IMAGE_3D_BINDING                  3

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
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
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

layout(early_fragment_tests) in;
layout(binding = DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];
layout(binding = COLOR_IMAGE_3D_BINDING_BASE, rgba8) coherent uniform image3D tColor;
layout(binding = NORMAL_IMAGE_3D_BINDING, rgba8_snorm) coherent uniform image3D tNormal;

in block
{
    vec3 position;
    vec3 normal;
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

void main()
{        
    //Voxel clean should happen before this stage
    ivec3 voxelPos = ivec3(vertexData.position*float(uResolution.x));
    //vec4 currentVoxelColor = imageLoad(tColor, voxelPos);
    //if (currentVoxelValue != vec4(0.0,0.0,0.0,0.0)
    //{
        //Right now this mimics a directional light that looks down the positive z axis
        vec4 material = getDiffuseColor(getMeshMaterial());
        vec3 color = material.rgb;
        float alpha = material.a;
        vec3 normal = normalize(vertexData.normal);
        vec3 position = vertexData.position; //normally you would have to multiply by the inverse viewprojection, but for now we are only rendering
        vec3 lightDirection = vec3(0.0,0.0,1.0); //normally you would have to convert the light direction to world space, but we are assuming it's looking straight on for now.
        float cosAngIncidence = clamp(dot(normal, lightDirection),0.0,1.0);
        vec3 reflectedDirection = reflect(-lightDirection, normal); //this is what you would use when computing specular
        vec3 lightColor = vec3(1.0,1.0,1.0); //normall this would be passed in to the shader
        vec3 finalColor = color*lightColor*cosAngIncidence; //this is what you would use for the diffuse color

        vec4 voxelColor = vec4(finalColor, alpha); //light color (and alpha)
        vec4 voxelNormal = vec4(reflectedDirection, uTimestamp); //light direction (magnitude contians specularity) and timestamp (w compontent already has the correct timestamp value). 
        imageStore(tColor, voxelPos, voxelColor);
        imageStore(tNormal, voxelPos, voxelNormal);
    //}
}
