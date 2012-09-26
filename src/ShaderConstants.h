#pragma once

#include <glf.hpp>
#include "Utils.h"

const uint POSITION_ATTR = 0;
const uint DEBUG_TRANSFORM_ATTR = 1;
const uint DEBUG_COLOR_ATTR = 2;
const uint PER_FRAME_UBO_BINDING = 0;
const uint VOXEL_TEXTURE_3D_BINDING = 0;

struct PerFrameUBO
{
    glm::mat4 viewProjection;
    glm::vec3 uCamLookAt;
    float padding1;
    glm::vec3 uCamPosition;
    float padding2;
    glm::vec3 uCamUp;
    float padding3;
    glm::uvec2 uResolution;
    float uTime;
};

#define SHADER_DIRECTORY std::string("src/shaders/")