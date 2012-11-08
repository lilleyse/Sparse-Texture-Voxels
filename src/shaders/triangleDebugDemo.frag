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

layout (location = 0, index = 0) out vec4 fragColor;


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

#define KD 0.6
#define KA 0.1
#define KS 0.3
#define SPEC 20
void main()
{
    vec3 position = vertexData.position;
    vec3 normal = normalize(vertexData.normal);
    vec3 color = getDiffuseColor(getMeshMaterial()).rgb;    
    float visibility = getVisibility();

    vec3 R = reflect(uLightDir, normal);
    vec3 V = normalize(position-uCamPos);

    vec3 cout = 
        KA * color + 
        KD * visibility*color*uLightColor*max(dot(uLightDir, normal), 0.0) + 
        KS * visibility*uLightColor*pow( max(dot(R,V), 0.0), SPEC );

    fragColor = vec4(cout, 1.0);
}
