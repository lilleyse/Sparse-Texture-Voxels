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
    vec2 uv;
    flat ivec2 propertyIndex;
} vertexData;

void main()
{    
    mat4 modelMatrix = getObjectPosition().modelMatrix; 
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    vec3 worldNormal = normalize(mat3(modelMatrix) * normal);
    gl_Position = uViewProjection * worldPosition;
    
    vertexData.position = vec3(worldPosition);
    vertexData.normal = worldNormal;
    vertexData.uv = uv;
    vertexData.propertyIndex = indexes;
}

