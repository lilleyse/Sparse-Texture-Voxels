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
#define SHADOW_MAP_BINDING                       3
#define DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    4     

// Image binding points

#define COLOR_IMAGE_3D_BINDING_BASE              0
#define COLOR_IMAGE_3D_BINDING_CURR              1
#define COLOR_IMAGE_3D_BINDING_NEXT              2
#define NORMAL_IMAGE_3D_BINDING                  3

// Framebuffer object outputs
#define DEFERRED_POSITIONS_FBO_BINDING       0
#define DEFERRED_COLORS_FBO_BINDING          1
#define DEFERRED_NORMALS_FBO_BINDING         2

// Shadow Map FBO
#define SHADOW_MAP_FBO_BINDING      0
#define BLURRED_MAP_FBO_BINDING     1

// Object properties
#define POSITION_INDEX        0
#define MATERIAL_INDEX        1

// Max values
#define MAX_TEXTURE_ARRAYS              10
#define FBO_BINDING_POINT_ARRAY_SIZE    4
#define NUM_OBJECTS_MAX                 500
#define NUM_MESHES_MAX                  500
#define MAX_POINT_LIGHTS                8

layout(std140, binding = PER_FRAME_UBO_BINDING) uniform PerFrameUBO
{
    mat4 uViewProjection;
    mat4 uLightView;
    mat4 uLightProj;
    vec3 uCamLookAt;
    vec3 uCamPos;
    vec3 uCamUp;
    vec3 uLightDir;
    vec3 uLightColor;
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

in block
{
    float depth;
} vertOutput;

layout (location = 0) out vec4 fragColor;

void main()
{    
    float depth = vertOutput.depth + .01;

    // Apply depth bias to avoid some z-fighting (maybe optional);
    //float dx = dFdx(depth);
	//float dy = dFdy(depth);
    //depth += abs(dx) + abs(dy);

    fragColor = vec4(depth, 0.0, 0.0, 0.0);
}
