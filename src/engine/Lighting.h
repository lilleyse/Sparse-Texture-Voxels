#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"

struct DirectionalLight
{
    glm::vec3 color;
    glm::vec3 direction;
    glm::mat4 projMatrix;
};

struct SpotLight
{
    float distance;
    glm::vec3 color;
    glm::vec3 position;
    glm::vec3 direction;
    float angle;
    glm::mat4 projMatrix;
};

struct PointLight
{
    glm::vec3 color;
    glm::vec3 position;
};


struct Lighting
{
    std::vector<DirectionalLight> dirLights;
    std::vector<SpotLight> spotLights;
    std::vector<PointLight> pointLights;
};