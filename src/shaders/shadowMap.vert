//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = POSITION_ATTR) in vec3 position;
layout(location = NORMAL_ATTR) in vec3 normal;
layout(location = UV_ATTR) in vec2 uv;
layout(location = PROPERTY_INDEX_ATTR)  in ivec2 indexes;

struct ObjectPosition
{
    mat4 modelMatrix;
};

// Wating to use Shader Storage Buffers so I don't need to statically declare my array size
layout(std140, binding = POSITION_ARRAY_BINDING) uniform PositionArray
{
    ObjectPosition positionArray[NUM_OBJECTS_MAX];
};

ObjectPosition getObjectPosition()
{
    int index = indexes[POSITION_INDEX];
    return positionArray[index];
}

struct MeshMaterial
{
    vec4 diffuseColor;
    vec4 specularColor;
    ivec2 textureLayer;
    float emission;
};

layout(std140, binding = MESH_MATERIAL_ARRAY_BINDING) uniform MeshMaterialArray
{
    MeshMaterial meshMaterialArray[NUM_MESHES_MAX];
};

MeshMaterial getMeshMaterial()
{
    int index = indexes[MATERIAL_INDEX];
    return meshMaterialArray[index];
}

out gl_PerVertex
{
    vec4 gl_Position;
};

out block
{
    float depth;
} vertOutput;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{    
    mat4 modelMatrix = getObjectPosition().modelMatrix; 
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    vec4 viewPosition = uLightView * worldPosition;
    gl_Position = uLightProj * viewPosition;

    // If the object is emissive, set is clip space w to 0 so that the vertex will be clipped
    // and so will not be rendered into shadow map

    gl_Position.w *= (1.0 - getMeshMaterial().emission);

    // Export depth pre-projection. Also apply scale of 0.1.
    vertOutput.depth = -viewPosition.z;
}

