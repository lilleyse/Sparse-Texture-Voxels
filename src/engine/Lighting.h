#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "Buffer.h"

struct BaseLight
{
    glm::vec3 color;
    float padding1;
    glm::vec3 ambient;
    float padding2;
};

struct DirectionalLight
{
    glm::vec3 direction;
    float padding;
    BaseLight base;
};

struct PointLight
{
    glm::vec4 position; //position.w contains attenuation factor
    BaseLight base;
};

struct Lighting
{
    PointLight pointLights[MAX_POINT_LIGHTS];
    DirectionalLight directionalLight;
    int numLights; float padding[3];
};

struct LightingHandler
{
    Lighting lighting;
    UniformBuffer* lightUBO;


    LightingHandler()
    {
        lighting.numLights = 0;
        lightUBO = new UniformBuffer(LIGHT_UBO_BINDING, 0, sizeof(Lighting), GL_STATIC_DRAW);
    }

    void addDirectionalLight(DirectionalLight& dirLight)
    {
        lighting.directionalLight = dirLight;
    }

    void addPointLight(PointLight& pointLight)
    {
        lighting.pointLights[lighting.numLights++];
    }

    void commitToGL()
    {
        lightUBO->commitToGL(&lighting, sizeof(Lighting));
    }
};