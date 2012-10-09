#pragma once

#include "../Utils.h"
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
        for(uint i = 0; i < objects.size(); i++)
        {
            delete objects[i];
        }
    }

    void addObject(RenderData& renderData, Object* object)
    {
        glm::vec3 translation = object->getTranslation();
        glm::vec3 scale = object->getScale();

        // Adjust object for a unit cube scene
        glm::vec3 worldSize = maxBounds - minBounds;
        glm::vec3 normalizedPosition = (translation - minBounds)/worldSize;
        object->setTranslation(normalizedPosition);

        glm::vec3 normalizedScale = scale/worldSize;
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

    void setBounds(glm::vec3& minBounds, glm::vec3& maxBounds)    
    {
        // Offset so that no clipping occurs if the bounding box is tight
        glm::vec3 offset(.1);
        this->minBounds = minBounds - offset;
        this->maxBounds = maxBounds + offset;
    }
};