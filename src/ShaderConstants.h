#pragma once

#include <glf.hpp>
#include "Utils.h"

const uint POSITION_ATTR = 0;
const uint NORMAL_ATTR = 1;
const uint DEBUG_TRANSFORM_ATTR = 2;
const uint DEBUG_COLOR_ATTR = 3;
const uint PER_FRAME_UBO_BINDING = 0;

const uint COLOR_TEXTURE_3D_BINDING = 0;
const uint NORMAL_TEXTURE_3D_BINDING = 1;
const uint DEFERRED_POSITIONS_TEXTURE_BINDING = 2;
const uint DEFERRED_COLORS_TEXTURE_BINDING = 3;
const uint DEFERRED_NORMALS_TEXTURE_BINDING = 4;
const uint NON_USED_TEXTURE = 5;

const uint MAX_FBO_BINDING_POINTS = 4;
const uint DEFERRED_POSITIONS_FBO_BINDING = 0;
const uint DEFERRED_COLORS_FBO_BINDING = 1;
const uint DEFERRED_NORMALS_FBO_BINDING = 2;



struct PerFrameUBO
{
    glm::mat4 viewProjection;
    glm::vec3 uCamLookAt;
    float padding1;
    glm::vec3 uCamPos;
    float padding2;
    glm::vec3 uCamUp;
    float padding3;
    glm::vec2 uResolution;
    float uAspect;
    float uTime;
    float uFOV;
};

#define SHADER_DIRECTORY std::string("src/shaders/")
