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

layout(binding = DIFFUSE_TEXTURE_ARRAY_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];
layout(binding = SHADOW_MAP_BINDING) uniform sampler2D shadowMap;  
layout(binding = VOXEL_DIRECTIONS_IMAGE_ARRAY_BINDING, r32ui) uniform uimage3D tDirectionalVoxels[NUM_VOXEL_DIRECTIONS];

struct MeshMaterial
{
    vec4 diffuseColor;
    vec4 specularColor;
    ivec2 diffuseTexture;
    ivec2 normalTexture;
    ivec2 specularTexture;
    float emission;
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

uint packColor(vec4 color)
{
    uvec4 cb = uvec4(color*255.0);
    return (cb.a << 24U) | (cb.b << 16U) | (cb.g << 8U) | cb.r;
}

void main()
{
    MeshMaterial material = getMeshMaterial();
    
    float visibility = getVisibility();
    vec4 diffuse = getDiffuseColor(material);
    float alpha = diffuse.a;
    vec3 normal = normalize(vertexData.normal);
    float LdotN = max(dot(uLightDir, normal), 0.0);
    vec3 outColor = diffuse.rgb*uLightColor*visibility*LdotN;

    // If emissive, ignore shading and just draw diffuse color
    outColor = mix(outColor, diffuse.rgb, material.emission);

    vec3 voxelBMin = uVoxelRegionWorld.xyz - uVoxelRegionWorld.w*(uCurrentCascade+1)/2.0; 
    vec3 voxelPosTextureSpace = (vertexData.position-voxelBMin)/(uVoxelRegionWorld.w*(uCurrentCascade+1));    // in tex coords
    ivec3 voxelPosImageCoord = ivec3(voxelPosTextureSpace * uVoxelRes);
    
    imageAtomicMax(tDirectionalVoxels[0], voxelPosImageCoord, packColor(vec4(outColor*max(normal.x, 0.0),  alpha)));
    imageAtomicMax(tDirectionalVoxels[1], voxelPosImageCoord, packColor(vec4(outColor*max(-normal.x, 0.0), alpha)));
    imageAtomicMax(tDirectionalVoxels[2], voxelPosImageCoord, packColor(vec4(outColor*max(normal.y, 0.0),  alpha)));
    imageAtomicMax(tDirectionalVoxels[3], voxelPosImageCoord, packColor(vec4(outColor*max(-normal.y, 0.0), alpha)));
    imageAtomicMax(tDirectionalVoxels[4], voxelPosImageCoord, packColor(vec4(outColor*max(normal.z, 0.0),  alpha)));
    imageAtomicMax(tDirectionalVoxels[5], voxelPosImageCoord, packColor(vec4(outColor*max(-normal.z, 0.0), alpha)));
}
