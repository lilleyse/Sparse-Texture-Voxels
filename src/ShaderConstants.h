#pragma once

#include <glf.hpp>

const unsigned int POSITION_ATTR = 0;
const unsigned int DEBUG_TRANSFORM_ATTR = 1;
const unsigned int DEBUG_COLOR_ATTR = 2;
const unsigned int PER_FRAME_UBO_BINDING = 0;
const unsigned int VOXEL_TEXTURE_3D_BINDING = 0;

struct PerFrameUBO
{
    glm::mat4 viewProjection;
    glm::vec3 uCamLookAt;
    float padding1;
    glm::vec3 uCamPos;
    float padding2;
    glm::vec3 uCamUp;
    float padding3;
    glm::uvec2 uResolution;
    float uTime;
};