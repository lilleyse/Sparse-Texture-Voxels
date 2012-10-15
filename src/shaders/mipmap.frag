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
#define NON_USED_TEXTURE                         0
#define COLOR_TEXTURE_3D_BINDING                 1
#define NORMAL_TEXTURE_3D_BINDING                2
#define DEFERRED_POSITIONS_TEXTURE_BINDING       3
#define DEFERRED_COLORS_TEXTURE_BINDING          4
#define DEFERRED_NORMALS_TEXTURE_BINDING         5
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    6      

// Image binding points
#define NON_USED_IMAGE                           0
#define NORMAL_IMAGE_3D_BINDING                  1
#define COLOR_IMAGE_3D_BINDING                   2         

// Framebuffer object outputs
#define DEFERRED_POSITIONS_FBO_BINDING       0
#define DEFERRED_COLORS_FBO_BINDING          1
#define DEFERRED_NORMALS_FBO_BINDING         2

// Object properties
#define POSITION_INDEX        0
#define MATERIAL_INDEX        1

// Max values
#define MAX_TEXTURE_ARRAYS              10
#define FBO_BINDING_POINT_ARRAY_SIZE    4
#define NUM_OBJECTS_MAX                 500
#define NUM_MESHES_MAX                  500
#define MAX_POINT_LIGHTS                8
#define MAX_3D_MIPMAP_LEVELS            10


layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 uViewProjection;
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
    vec2 uResolution;
    float uAspect;
    float uTime;
    float uFOV;
};


//---------------------------------------------------------
// SHADER VARS
//---------------------------------------------------------

layout(location = 0) out vec4 fragColor;

layout(binding = COLOR_IMAGE_3D_BINDING, rgba8) uniform image3D tColorMips[MAX_3D_MIPMAP_LEVELS];

flat in int slice;

void main()
{
    //fragColor = vec4(float(slice)/32.0);

    ivec3 indexNext = ivec3(ivec2(gl_FragCoord.xy), slice);
    ivec3 indexCurr = indexNext*2;

    vec4 color = vec4(1.0, 1.0, 0.0, 1.0);
    imageStore(tColorMips[1], indexNext, color);
}