//---------------------------------------------------------
// VOXELIZER
//---------------------------------------------------------

//---------------------------------------------------------
// GL IN/OUT
//---------------------------------------------------------

in block
{
    vec3 position;
    vec3 normal;
    vec3 shadowMapPos;
    vec2 uv;
    flat ivec2 propertyIndex;
} vertexData;

//---------------------------------------------------------
// GLOBAL DATA
//---------------------------------------------------------

layout(binding = DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];
layout(binding = SHADOW_MAP_BINDING) uniform sampler2D shadowMap;  

layout(binding = COLOR_IMAGE_POSX_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosX;
layout(binding = COLOR_IMAGE_NEGX_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegX;
layout(binding = COLOR_IMAGE_POSY_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosY;
layout(binding = COLOR_IMAGE_NEGY_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegY;
layout(binding = COLOR_IMAGE_POSZ_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorPosZ;
layout(binding = COLOR_IMAGE_NEGZ_3D_BINDING, rgba8) writeonly uniform image3D tVoxColorNegZ;

struct MeshMaterial
{
    vec4 diffuseColor;
    vec4 specularColor;
    ivec2 diffuseTexture;
    ivec2 normalTexture;
    ivec2 specularTexture;
};

layout(std140, binding = MESH_MATERIAL_ARRAY_BINDING) uniform MeshMaterialArray
{
    MeshMaterial meshMaterialArray[NUM_MESHES_MAX];
};

//---------------------------------------------------------
// COMMON SHADING
//---------------------------------------------------------

MeshMaterial getMeshMaterial()
{
    int index = vertexData.propertyIndex[MATERIAL_INDEX];
    return meshMaterialArray[index];
}

vec4 getDiffuseColor(MeshMaterial material)
{
    int textureId = material.diffuseTexture.x;
    int textureLayer = material.diffuseTexture.y;

    if(textureId == -1) // no texture
        return material.diffuseColor;
    
    vec4 diffuseColor = texture(diffuseTextures[textureId], vec3(vertexData.uv, textureLayer));
    
    if(diffuseColor.a == 0.0) // no alpha = invisible and discarded
        discard;
    return diffuseColor;
}

float getVisibility()
{
	float fragLightDepth = vertexData.shadowMapPos.z;
    float shadowMapDepth = texture(shadowMap, vertexData.shadowMapPos.xy).r;

    if(fragLightDepth <= shadowMapDepth)
        return 1.0;

    // Less darknessFactor means lighter shadows
    float darknessFactor = 20.0;
    return clamp(exp(darknessFactor * (shadowMapDepth - fragLightDepth)), 0.0, 1.0);
}

//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    float visibility = getVisibility();
    vec4 diffuse = getDiffuseColor(getMeshMaterial());
    vec3 normal = normalize(vertexData.normal);
    float LdotN = max(dot(uLightDir, normal), 0.0);
    vec4 outColor = vec4(1.0,0.0,0.0,1.0);//diffuse.rgb*uLightColor*visibility*LdotN;

    // six directions
    float AdotNPosX = max(normal.x, 0.0);
    float AdotNNegX = max(-normal.x, 0.0);
    float AdotNPosY = max(normal.y, 0.0);
    float AdotNNegY = max(-normal.y, 0.0);
    float AdotNPosZ = max(normal.z, 0.0);
    float AdotNNegZ = max(-normal.z, 0.0);

    // write to image
    vec3 position = vertexData.position;
    ivec3 voxelPos = ivec3(vertexData.position*float(uVoxelRes.x));
    imageStore(tVoxColorPosX, voxelPos, outColor);
    imageStore(tVoxColorNegX, voxelPos, outColor);
    imageStore(tVoxColorPosY, voxelPos, outColor);
    imageStore(tVoxColorNegY, voxelPos, outColor);
    imageStore(tVoxColorPosZ, voxelPos, outColor);
    imageStore(tVoxColorNegZ, voxelPos, outColor);
}
