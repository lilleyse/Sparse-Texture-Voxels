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

out gl_PerVertex
{
    vec4 gl_Position;
};

out block
{
    vec3 position;
    vec3 normal;
    vec3 shadowMapPos;
    vec2 uv;
    flat ivec2 propertyIndex;
} vertexData;


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{    
    mat4 modelMatrix = getObjectPosition().modelMatrix; 
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    vec3 worldNormal = normalize(mat3(modelMatrix) * normal);
    gl_Position = uViewProjection * worldPosition;
    
    vec4 lightViewPos = uLightView * worldPosition;
    vec4 lightProjPos = uLightProj * lightViewPos;
    // Offset from NDC to texture space
    vertexData.shadowMapPos.xy = (lightProjPos.xy / lightProjPos.w) * 0.5 + 0.5;
    vertexData.shadowMapPos.z = -lightViewPos.z;

    vertexData.position = vec3(worldPosition);
    vertexData.normal = worldNormal;
    vertexData.uv = uv;
    vertexData.propertyIndex = indexes;
}

