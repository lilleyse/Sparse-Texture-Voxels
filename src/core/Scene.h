#pragma once
#include <glf.hpp>
#include "Object.h"
#include "Mesh.h"
#include "RenderData.h"
#include "Lighting.h"

struct Scene
{
    std::vector<Object*> objects;
    LightingHandler lightingHandler;
    glm::vec3 minBounds;
    glm::vec3 maxBounds;

    Scene()
    {
        // Nothing
    }

    ~Scene()
    {
        //delete all Objects
        for(unsigned int i = 0; i < objects.size(); i++)
        {
            delete objects[i];
        }
    }

    void addObject(RenderData& renderData, Object* object)
    {
        // Adjust object for a unit cube scene
        glm::vec3 worldSize = maxBounds - minBounds;
        glm::vec3 normalizedPosition = (object->getTranslation() - minBounds)/worldSize;
        object->setTranslation(normalizedPosition);

        glm::vec3 normalizedScale = object->getScale()/worldSize;
        object->setScale(normalizedScale);

        // Rotation should be unmodified

        objects.push_back(object);
        renderData.addObject(object);
    }

    void addDirectionalLight(DirectionalLight& dirLight)
    {
        //lightingHandler.addDirectionalLight(dirLight);
    }

    void addPointLight(PointLight& pointLight)
    {
        //lightingHandler.addPointLight(pointLight);
    }

    void commitToGL()
    {
        //lightingHandler.commitToGL();
    }
};