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
#define NON_USED_TEXTURE                         0 // Used for modifying textures that shouldn't be bound to a binding point
#define COLOR_TEXTURE_3D_BINDING                 1
#define NORMAL_TEXTURE_3D_BINDING                2
#define DEFERRED_POSITIONS_TEXTURE_BINDING       3
#define DEFERRED_COLORS_TEXTURE_BINDING          4
#define DEFERRED_NORMALS_TEXTURE_BINDING         5
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    6
#define RESERVED_TEXTURE_ARRAY_1                 7
#define RESERVED_TEXTURE_ARRAY_2                 8
#define RESERVED_TEXTURE_ARRAY_3                 9
#define RESERVED_TEXTURE_ARRAY_4                 10
#define RESERVED_TEXTURE_ARRAY_5                 11
#define RESERVED_TEXTURE_ARRAY_6                 12
#define RESERVED_TEXTURE_ARRAY_7                 13
#define RESERVED_TEXTURE_ARRAY_8                 14
#define RESERVED_TEXTURE_ARRAY_9                 15

// Image binding points
#define COLOR_IMAGE_3D_BINDING_BASE              0
#define COLOR_IMAGE_3D_BINDING_CURR              1
#define COLOR_IMAGE_3D_BINDING_NEXT              2
#define NORMAL_IMAGE_3D_BINDING                  3

// Framebuffer object outputs
#define DEFERRED_POSITIONS_FBO_BINDING       0
#define DEFERRED_COLORS_FBO_BINDING          1
#define DEFERRED_NORMALS_FBO_BINDING         2

// Object properties
#define POSITION_INDEX        0
#define MATERIAL_INDEX        1

// Max values
#define MAX_TEXTURE_ARRAYS               10
#define FBO_BINDING_POINT_ARRAY_SIZE     3
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
    int uTimestamp;
    float uFOV;
    float uTextureRes;
    float uNumMips;
};


layout(location = POSITION_ATTR) in vec3 position;
layout(location = NORMAL_ATTR) in vec3 normal;
layout(location = DEBUG_TRANSFORM_ATTR) in vec4 transformation;
layout(location = DEBUG_COLOR_ATTR) in vec4 color;

out block
{
    vec3 position;
    vec4 color;
    vec3 normal;

} vertexData;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    // Create the model matrix
    float scale = transformation.w;
    mat4 modelMatrix = mat4(scale);
    modelMatrix[3] = vec4(transformation.xyz, 1.0);

    vec4 worldPosition = modelMatrix * vec4(position, 1.0);

    // Caluclate the clip space position
    gl_Position = uViewProjection * worldPosition;
    
    vertexData.position = vec3(worldPosition);
    vertexData.color = color;
    vertexData.normal = normal;

}