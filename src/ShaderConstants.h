#pragma once

#include "Utils.h"

// Vertex attribute indexes
const uint POSITION_ATTR            = 0;
const uint NORMAL_ATTR              = 1;
const uint UV_ATTR                  = 2;
const uint PROPERTY_INDEX_ATTR      = 3;
const uint DEBUG_TRANSFORM_ATTR     = 4;
const uint DEBUG_COLOR_ATTR         = 5;

// Uniform buffer objects binding points
const uint PER_FRAME_UBO_BINDING            = 0;
const uint LIGHT_UBO_BINDING                = 1;
const uint MESH_MATERIAL_ARRAY_BINDING      = 2;
const uint POSITION_ARRAY_BINDING           = 3;

// Sampler binding points
const uint NON_USED_TEXTURE                         = 0; // Used for modifying textures that shouldn't be bound to a binding point
const uint COLOR_TEXTURE_POSX_3D_BINDING            = 1; // right direction
const uint COLOR_TEXTURE_NEGX_3D_BINDING            = 2; // left direction
const uint COLOR_TEXTURE_POSY_3D_BINDING            = 3; // up direc tion
const uint COLOR_TEXTURE_NEGY_3D_BINDING            = 4; // down direction
const uint COLOR_TEXTURE_POSZ_3D_BINDING            = 5; // front direction
const uint COLOR_TEXTURE_NEGZ_3D_BINDING            = 6; // back direction
const uint SHADOW_MAP_BINDING                       = 7;
const uint DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    = 8;
const uint RESERVED_TEXTURE_ARRAY_1                 = 9;
const uint RESERVED_TEXTURE_ARRAY_2                 = 10;
const uint RESERVED_TEXTURE_ARRAY_3                 = 11;
const uint RESERVED_TEXTURE_ARRAY_4                 = 12;
const uint RESERVED_TEXTURE_ARRAY_5                 = 13;
const uint RESERVED_TEXTURE_ARRAY_6                 = 14;
const uint RESERVED_TEXTURE_ARRAY_7                 = 15;
const uint RESERVED_TEXTURE_ARRAY_8                 = 16;
const uint RESERVED_TEXTURE_ARRAY_9                 = 17;

// Image binding points
const uint COLOR_IMAGE_POSX_3D_BINDING              = 0; // right direction 
const uint COLOR_IMAGE_NEGX_3D_BINDING              = 1; // left direction
const uint COLOR_IMAGE_POSY_3D_BINDING              = 2; // up direction
const uint COLOR_IMAGE_NEGY_3D_BINDING              = 3; // down direction
const uint COLOR_IMAGE_POSZ_3D_BINDING              = 4; // front direction
const uint COLOR_IMAGE_NEGZ_3D_BINDING              = 5; // back direction

// Shadow Map FBO
const uint SHADOW_MAP_FBO_BINDING = 0;
const uint BLURRED_MAP_FBO_BINDING = 1;

// Object properties
const int POSITION_INDEX        = 0;
const int MATERIAL_INDEX        = 1;

// Max values
const uint MAX_TEXTURE_ARRAYS               = 10;
const uint NUM_OBJECTS_MAX                  = 500;
const uint NUM_MESHES_MAX                   = 500;
const uint MAX_POINT_LIGHTS                 = 8;

struct PerFrameUBO
{
    glm::mat4 uViewProjection;
    glm::mat4 uWorldToShadowMap;
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
    glm::vec2 uResolution;
    float uAspect;
    float uTime;
    float uTimestamp;
    float uFOV;
    float uTextureRes;
    float uNumMips;
    float uSpecularFOV;
    float uSpecularAmount;
    int uCurrentMipLevel;
};
