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
const uint COLOR_TEXTURE_3D_BINDING                 = 1;
const uint NORMAL_TEXTURE_3D_BINDING                = 2;
const uint SHADOW_MAP_BINDING                       = 3;
const uint DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING    = 4;
const uint RESERVED_TEXTURE_ARRAY_1                 = 5;
const uint RESERVED_TEXTURE_ARRAY_2                 = 6;
const uint RESERVED_TEXTURE_ARRAY_3                 = 7;
const uint RESERVED_TEXTURE_ARRAY_4                 = 8;
const uint RESERVED_TEXTURE_ARRAY_5                 = 9;
const uint RESERVED_TEXTURE_ARRAY_6                 = 10;
const uint RESERVED_TEXTURE_ARRAY_7                 = 11;
const uint RESERVED_TEXTURE_ARRAY_8                 = 12;
const uint RESERVED_TEXTURE_ARRAY_9                 = 13;

// Image binding points
const uint COLOR_IMAGE_3D_BINDING_BASE              = 0;
const uint COLOR_IMAGE_3D_BINDING_CURR              = 1;
const uint COLOR_IMAGE_3D_BINDING_NEXT              = 2;
const uint NORMAL_IMAGE_3D_BINDING                  = 3;

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
    glm::mat4 uWorldToLight;
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
};
