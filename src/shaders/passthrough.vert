layout(location = POSITION_ATTR) in vec3 position;
layout(location = NORMAL_ATTR) in vec3 normal;
layout(location = UV_ATTR) in vec2 uv;
//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

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


//---------------------------------------------------------
// PROGRAM
//---------------------------------------------------------

void main()
{    
    mat4 modelMatrix = getObjectPosition().modelMatrix; 
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    gl_Position = uViewProjection * worldPosition;
}

