#pragma once

#include <tinyxml2.h>

#include "../Utils.h"
#include "Scene.h"
#include "MeshLibrary.h"
#include "ShaderLibrary.h"

using namespace tinyxml2;

struct SceneLibrary
{
    void begin()
    {
        // Nothing
    }

    glm::vec3 getVec3FromString(std::string& vecString)
    {
        glm::vec3 result;
        std::vector<std::string> pieces = Utils::parseSpaceSeparatedString(vecString);
        result.x = (float)std::atof(pieces[0].c_str());
        result.y = (float)std::atof(pieces[1].c_str());
        result.z = (float)std::atof(pieces[2].c_str());
        return result;
    }

    glm::vec4 getVec4FromString(std::string& vecString)
    {
        glm::vec4 result;
        std::vector<std::string> pieces = Utils::parseSpaceSeparatedString(vecString);
        result.x = (float)std::atof(pieces[0].c_str());
        result.y = (float)std::atof(pieces[1].c_str());
        result.z = (float)std::atof(pieces[2].c_str());
        result.w = (float)std::atof(pieces[3].c_str());
        return result;
    }

    glm::vec3 getTranslation(XMLElement* translateElement)
    {
        std::string translateString = translateElement->FirstChild()->Value();
        return getVec3FromString(translateString);
    }

    glm::vec3 getScale(XMLElement* scaleElement)
    {
        std::string scaleString = scaleElement->FirstChild()->Value();
        return getVec3FromString(scaleString);
    }

    // returns axis followed by angle
    glm::vec4 getRotation(XMLElement* rotateElement) 
    {
        std::string rotateString = rotateElement->FirstChild()->Value();
        return getVec4FromString(rotateString);
    }


    Scene* addScene(RenderData& renderData, MeshLibrary& meshLibrary, ShaderLibrary& shaderLibrary, std::string name, std::string filename)
    {
        // Open XML document
        XMLDocument doc;
        doc.LoadFile(filename.c_str());
        if(doc.Error())
                return 0;
        XMLHandle docHandle(&doc);
        XMLElement* header = docHandle.FirstChildElement("Scene").ToElement();

        XMLElement* infoElement = header->FirstChildElement("info");


        Scene* scene = new Scene();

        // Get scene name
        std::string sceneName = infoElement->FirstChildElement("name")->FirstChild()->Value();
       
        // Get bottom corner bounds
        std::string bottomCornerString = infoElement->FirstChildElement("bottom_corner")->FirstChild()->Value();
        glm::vec3 bottomCorner = getVec3FromString(bottomCornerString);

        // Get top corner bounds
        std::string topCornerString = infoElement->FirstChildElement("top_corner")->FirstChild()->Value();
        glm::vec3 topCorner = getVec3FromString(topCornerString);

        scene->setBounds(bottomCorner, topCorner);

        // Loop over meshes and load them
        XMLElement* meshesElement = header->FirstChildElement("meshes");
        for(XMLElement* meshElement = meshesElement->FirstChildElement("mesh"); meshElement != 0; meshElement = meshElement->NextSiblingElement("mesh"))
        {
            std::string meshName = meshElement->Attribute("name");
            std::string meshFilename = MESH_DIRECTORY + meshName + ".xml";
            meshLibrary.loadMeshFile(meshName, meshFilename);
        }

        // Loop over objects and create them
        XMLElement* objectsElement = header->FirstChildElement("objects");
        for(XMLElement* objectElement = objectsElement->FirstChildElement("object"); objectElement != 0; objectElement = objectElement->NextSiblingElement("object"))
        {
            // Get object name
            std::string objectName = objectElement->FirstChildElement("name")->FirstChild()->Value();

            // Get mesh name and retrieve mesh
            std::string meshName = objectElement->FirstChildElement("mesh")->FirstChild()->Value();
            Mesh* mesh = meshLibrary.get(meshName);

            // Create object
            Object* object = new Object(mesh, shaderLibrary.debugDrawShader);   

            // Get translation and apply it
            XMLElement* translateElement = objectElement->FirstChildElement("translate");
            glm::vec3 translate = getTranslation(translateElement);
            object->translate(translate);
           
            // Get scale and apply it
            XMLElement* scaleElement = objectElement->FirstChildElement("scale");
            glm::vec3 scale = getScale(scaleElement);
            object->scale(scale);

            // Get rotation and apply it 
            XMLElement* rotateElement = objectElement->FirstChildElement("rotate");
            glm::vec4 rotation = getRotation(rotateElement);
            object->rotate(glm::vec3(rotation), rotation.w); 

            scene->addObject(renderData, object);
        }

        

        // Loop over lights and create them
        XMLElement* lightsElement = header->FirstChildElement("lights");
        if(lightsElement)
        {
            for(XMLElement* lightElement = lightsElement->FirstChildElement("light"); lightElement != 0; lightElement = lightElement->NextSiblingElement("light"))
            {
                // Get color (all light types have a color)
                std::string colorString = lightElement->FirstChildElement("color")->FirstChild()->Value();
                glm::vec3 color = getVec3FromString(colorString);

                // Get the light type
                std::string lightType = lightElement->Attribute("type");

                // Loop over light types
                if(lightType == "Sun")
                {
                    // Get direction
                    std::string directionString = lightElement->FirstChildElement("direction")->FirstChild()->Value();
                    glm::vec3 direction = getVec3FromString(directionString);

                    DirectionalLight dirLight;
                    dirLight.color = color;
                    dirLight.direction = direction;
                    scene->addDirectionalLight(dirLight);
                }
                else if(lightType == "Point")
                {
                    // Get position
                    std::string positionString = lightElement->FirstChildElement("translate")->FirstChild()->Value();
                    glm::vec3 position = getVec3FromString(positionString);

                    PointLight pointLight;
                    pointLight.color = color;
                    pointLight.position = position;
                    scene->addPointLight(pointLight);
                }
                else if(lightType == "Spot")
                {
                    // Get distance and angle
                    float distance = (float)atof(lightElement->FirstChildElement("distance")->FirstChild()->Value());
                    float angle = (float)atof(lightElement->FirstChildElement("angle")->FirstChild()->Value());
                    
                    // Get position
                    std::string positionString = lightElement->FirstChildElement("translate")->FirstChild()->Value();
                    glm::vec3 position = getVec3FromString(positionString);
                 
                    // Get direction
                    std::string directionString = lightElement->FirstChildElement("direction")->FirstChild()->Value();
                    glm::vec3 direction = getVec3FromString(directionString);

                    SpotLight spotLight;
                    spotLight.color = color;
                    spotLight.position = position;
                    spotLight.distance = distance;
                    spotLight.angle = angle;
                    spotLight.direction = direction;
                    scene->addSpotLight(spotLight);
                }
            }
        }

        return scene;
    }
};