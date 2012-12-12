// The shader version of this file is in src/shaders/globals

#pragma once

#include "Utils.h"

// Max values
const uint NUM_VOXEL_DIRECTIONS             = 6;
const uint MAX_VOXEL_CASCADES               = 3;
const uint MAX_VOXEL_TEXTURES               = 18; // directions*cascades
const uint MAX_TEXTURE_ARRAYS               = 10;
const uint NUM_OBJECTS_MAX                  = 500;
const uint NUM_MESHES_MAX                   = 500;
const uint MAX_POINT_LIGHTS                 = 8;

// Vertex attribute indexes
const uint POSITION_ATTR                          = 0;
const uint NORMAL_ATTR                            = 1;
const uint UV_ATTR                                = 2;
const uint PROPERTY_INDEX_ATTR                    = 3;
const uint DEBUG_TRANSFORM_ATTR                   = 4;
const uint DEBUG_COLOR_ATTR[NUM_VOXEL_DIRECTIONS] = {5,6,7,8,9,10};

// Uniform buffer objects binding points
const uint PER_FRAME_UBO_BINDING       = 0;
const uint LIGHT_UBO_BINDING           = 1;
const uint MESH_MATERIAL_ARRAY_BINDING = 2;
const uint POSITION_ARRAY_BINDING      = 3;

// Sampler binding points
const uint NON_USED_TEXTURE                                     = 0; // Used for modifying textures that shouldn't be bound to a binding point
const uint SHADOW_MAP_BINDING                                   = 1;
const uint VOXEL_DIRECTIONS_ARRAY_BINDING[MAX_VOXEL_TEXTURES]   = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
const uint DIFFUSE_TEXTURE_ARRAY_BINDING[MAX_TEXTURE_ARRAYS]    = {20,21,22,23,24,25,26,27,28,29};

// Image binding points
const uint VOXEL_DIRECTIONS_IMAGE_ARRAY_BINDING[NUM_VOXEL_DIRECTIONS] = {0,1,2,3,4,5};

// Shadow Map FBO
const uint SHADOW_MAP_FBO_BINDING  = 0;
const uint BLURRED_MAP_FBO_BINDING = 1;

// Object properties
const int POSITION_INDEX        = 0;
const int MATERIAL_INDEX        = 1;

struct PerFrameUBO
{
    glm::mat4 uViewProjection;
    glm::mat4 uLightView;
    glm::mat4 uLightProj;
    glm::vec3 uCamLookAt;
    float padding1;
    glm::vec3 uCamPos;
    float padding2;
    glm::vec3 uCamUp;
    float padding3;
    glm::vec3 uLightDir;
    float padding4;
    glm::vec3 uLightColor;
    float padding5;
    glm::vec4 uVoxelRegionWorld[MAX_VOXEL_CASCADES]; //.xyz is origin and .w is size
    glm::vec2 uScreenRes;
    float uAspect;
    float uTime;
    float uTimestamp;
    float uFOV;
    float uVoxelRes;
    float uNumMips;
    float uSpecularFOV;
    float uSpecularAmount;
    int uCurrentMipLevel;
    int uCurrentCascade;
};
