//---------------------------------------------------------
// TRIANGLE ENGINE
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

layout (location = 0, index = 0) out vec4 fragColor;


//---------------------------------------------------------
// GLOBAL DATA
//---------------------------------------------------------

layout(binding = DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING) uniform sampler2DArray diffuseTextures[MAX_TEXTURE_ARRAYS];
layout(binding = SHADOW_MAP_BINDING) uniform sampler2D shadowMap; 

struct MeshMaterial
{
    vec4 diffuseColor;
    vec4 specularColor;
    ivec2 diffuseTexture;
    ivec2 normalTexture;
    ivec2 specularTexture;
    float emissive;
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

vec3 getNormal(MeshMaterial material, vec3 vertexNormal)
{
    int textureId = material.normalTexture.x;
    int textureLayer = material.normalTexture.y;
    
    if(textureId == -1) // no texture
        return vertexNormal;

    // Get bump map normal
    vec3 bumpMapNormal = texture(diffuseTextures[textureId], vec3(vertexData.uv, textureLayer)).xyz;
    // Create tangent, bitangent, normal matrix
    // compute derivations of the texture coordinate
    vec2 tc_dx = dFdx(vertexData.uv);
    vec2 tc_dy = dFdy(vertexData.uv);
    vec3 p_dx  = dFdx(vertexData.position);
    vec3 p_dy  = dFdy(vertexData.position);
    // compute initial tangent and bi-tangent
    vec3 t = normalize( tc_dy.y * p_dx - tc_dx.y * p_dy );
    vec3 b = normalize( tc_dy.x * p_dx - tc_dx.x * p_dy ); // sign inversion
    // get new tangent from a given mesh normal
    vec3 n = vertexNormal;
    vec3 x = cross(n, t);
    t = cross(x, n);
    t = normalize(t);
    // get updated bi-tangent
    x = cross(b, n);
    b = cross(n, x);
    b = normalize(b);
    mat3 tbn = mat3(t, b, n);

    // Converted bump map normal to [-1,1] range
    bumpMapNormal = 2.0 * bumpMapNormal - vec3(1.0, 1.0, 1.0);
    vec3 newNormal = tbn * bumpMapNormal;
    newNormal = normalize(newNormal);
    return newNormal;
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

vec3 getSpecularColor(MeshMaterial material)
{
    int textureId = material.specularTexture.x;
    int textureLayer = material.specularTexture.y;

    if(textureId == -1) // no texture
        return material.specularColor.rgb;
    else
        return texture(diffuseTextures[textureId], vec3(vertexData.uv, textureLayer)).rgb;
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
// SHADER VARS
//---------------------------------------------------------

#define KD 0.6
#define KA 0.2
#define KS 0.3
#define SPEC 0.2


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{
    MeshMaterial material = getMeshMaterial();
    vec3 position = vertexData.position;
    vec3 normal = getNormal(material, normalize(vertexData.normal));
    vec3 color = getDiffuseColor(material).rgb;  
    vec3 specularColor = getSpecularColor(material);

    float visibility = getVisibility();

    vec3 reflectedLight = reflect(uLightDir, normal);
    vec3 view = normalize(position-uCamPos);
    float diffuseTerm = max(dot(uLightDir, normal), 0.0);
    
    //gaussian (might be able to do it faster - unreal slides page 31)
    vec3 halfAngle = normalize(uLightDir - view);
    float angleNormalHalf = acos(dot(halfAngle, normal));
    float exponent = angleNormalHalf / SPEC;
    exponent = -(exponent * exponent);
    float specularTerm = diffuseTerm != 0.0 ? exp(exponent) : 0.0;

    //phong
    //float specularTerm = pow( max(dot(reflectedLight,view), 0.0), SPEC );

    vec3 cout = 
        KA * color + 
        KD * visibility*color*uLightColor*diffuseTerm +
        KS * visibility*specularColor*uLightColor*specularTerm;

    // If emissive, ignore shading and just draw diffuse color
    cout = mix(cout, color, material.emissive);

    fragColor = vec4(cout, 1.0);
}


