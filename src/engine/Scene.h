#pragma once

#include "../Utils.h"
#include "Object.h"
#include "Mesh.h"
#include "RenderData.h"
#include "Lighting.h"

struct Scene
{
    std::vector<Object*> objects;
    Lighting lighting;
    Object* lightObject;
    glm::vec3 minBounds;
    glm::vec3 maxBounds;
    float radius;

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
        //glm::vec3 translation = object->getTranslation();
        //glm::vec3 scale = object->getScale();

        // Adjust object for a unit cube scene
        //glm::vec3 worldSize = maxBounds - minBounds;
        //glm::vec3 normalizedPosition = (translation - minBounds)/worldSize;
        //object->setTranslation(normalizedPosition);

        //glm::vec3 normalizedScale = scale/worldSize;
        //object->setScale(normalizedScale);

        // Rotation should be unmodified


        objects.push_back(object);
        renderData.addObject(object);
    }

    void addDirectionalLight(DirectionalLight& dirLight)
    {
        lighting.dirLights.push_back(dirLight);
    }

    void addPointLight(PointLight& pointLight)
    {
        lighting.pointLights.push_back(pointLight);
    }

    void addSpotLight(SpotLight& spotLight)
    {
        lighting.spotLights.push_back(spotLight);
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

        this->radius = glm::length((maxBounds - minBounds)/2.0f);
    }

    void display(RenderData& renderData)
    {
        for(uint i = 0; i < objects.size(); i++)
        {
            Object* object = objects[i];
            if(object->dirtyPosition)
            {
                // No longer dirty
                object->dirtyPosition = false;
                renderData.updateObject(object);
            }
        }

    }
};