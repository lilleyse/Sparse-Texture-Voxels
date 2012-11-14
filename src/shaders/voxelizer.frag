//---------------------------------------------------------
// TRIANGLE ENGINE
//---------------------------------------------------------

layout(binding = DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];

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


//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(binding = SHADOW_MAP_BINDING) uniform sampler2D shadowMap;  
layout(binding = COLOR_IMAGE_POSX_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosX;
layout(binding = COLOR_IMAGE_NEGX_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegX;
layout(binding = COLOR_IMAGE_POSY_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosY;
layout(binding = COLOR_IMAGE_NEGY_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegY;
layout(binding = COLOR_IMAGE_POSZ_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosZ;
layout(binding = COLOR_IMAGE_NEGZ_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegZ;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

float getVisibility()
{
    float fragLightDepth = vertexData.shadowMapPos.z;
    float shadowMapDepth = texture(shadowMap, vertexData.shadowMapPos.xy).r;

    if(fragLightDepth <= shadowMapDepth)
        return 1.0;

    // Less darknessFactor means lighter shadows
    float darknessFactor = 50.0;
    return clamp(exp(darknessFactor * (shadowMapDepth - fragLightDepth)), 0.0, 1.0);
}

void main()
{
    float visibility = getVisibility();
    vec4 diffuse = getDiffuseColor(getMeshMaterial());
    vec3 normal = normalize(vertexData.normal);
    float LdotN = max(dot(uLightDir, normal), 0.0);
    vec3 outColor = diffuse.rgb*uLightColor*visibility*LdotN;

    // six directions
    float AdotNPosX = max(normal.x, 0.0);
    float AdotNNegX = max(-normal.x, 0.0);
    float AdotNPosY = max(normal.y, 0.0);
    float AdotNNegY = max(-normal.y, 0.0);
    float AdotNPosZ = max(normal.z, 0.0);
    float AdotNNegZ = max(-normal.z, 0.0);

    // write to image
    vec3 position = vertexData.position;
    ivec3 voxelPos = ivec3(vertexData.position*float(uResolution.x));
    //imageStore(tVoxColorPosX, voxelPos, vec4(outColor*AdotNPosX, diffuse.a));
    //imageStore(tVoxColorNegX, voxelPos, vec4(outColor*AdotNNegX, diffuse.a));
    //imageStore(tVoxColorPosY, voxelPos, vec4(outColor*AdotNPosY, diffuse.a));
    //imageStore(tVoxColorNegY, voxelPos, vec4(outColor*AdotNNegY, diffuse.a));
    //imageStore(tVoxColorPosZ, voxelPos, vec4(outColor*AdotNPosZ, diffuse.a));
    //imageStore(tVoxColorNegZ, voxelPos, vec4(outColor*AdotNNegZ, diffuse.a));

    imageStore(tVoxColorPosX, voxelPos, vec4(outColor, diffuse.a));
    imageStore(tVoxColorNegX, voxelPos, vec4(outColor, diffuse.a));
    imageStore(tVoxColorPosY, voxelPos, vec4(outColor, diffuse.a));
    imageStore(tVoxColorNegY, voxelPos, vec4(outColor, diffuse.a));
    imageStore(tVoxColorPosZ, voxelPos, vec4(outColor, diffuse.a));
    imageStore(tVoxColorNegZ, voxelPos, vec4(outColor, diffuse.a));
}
