#pragma once
#include "Scene.h"
#include <tinyxml2.h>
#include "../Utils.h"
#include "MeshLibrary.h"
#include "ShaderLibrary.h"
using namespace tinyxml2;

struct SceneLibrary
{
    void begin()
    {
        // Nothing
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
        glm::vec3 bottomCorner;
        std::string bottomCornerString = infoElement->FirstChildElement("bottom_corner")->FirstChild()->Value();
        std::vector<std::string> bottomCornerPieces = Utils::parseSpaceSeparatedString(bottomCornerString);
        bottomCorner.x = (float)std::atof(bottomCornerPieces[0].c_str());
        bottomCorner.y = (float)std::atof(bottomCornerPieces[1].c_str());
        bottomCorner.z = (float)std::atof(bottomCornerPieces[2].c_str());

        // Get top corner bounds
        glm::vec3 topCorner;
        std::string topCornerString = infoElement->FirstChildElement("top_corner")->FirstChild()->Value();
        std::vector<std::string> topCornerPieces = Utils::parseSpaceSeparatedString(topCornerString);
        topCorner.x = (float)std::atof(topCornerPieces[0].c_str());
        topCorner.y = (float)std::atof(topCornerPieces[1].c_str());
        topCorner.z = (float)std::atof(topCornerPieces[2].c_str());

        scene->minBounds = bottomCorner;
        scene->maxBounds = topCorner;

        // Loop over meshes and load them
        XMLElement* meshesElement = header->FirstChildElement("meshes");
        for(XMLElement* meshElement = meshesElement->FirstChildElement("mesh"); meshElement != 0; meshElement = meshElement->NextSiblingElement("mesh"))
        {
            std::string meshName = meshElement->Attribute("name");
            std::string meshFilename = MESH_DIRECTORY + meshName + ".xml";
            meshLibrary.loadMeshFile(meshName, meshFilename);
        }

        // Loop over objects and create them
        for(XMLElement* objectElement = header->FirstChildElement("object"); objectElement != 0; objectElement = objectElement->NextSiblingElement("object"))
        {
            // Get object name
            std::string objectName = objectElement->FirstChildElement("name")->FirstChild()->Value();

            // Get mesh name and retrieve mesh
            std::string meshName = objectElement->FirstChildElement("mesh")->FirstChild()->Value();
            Mesh* mesh = meshLibrary.get(meshName);

            // Create object
            Object* object = new Object(mesh, shaderLibrary.debugDrawShader);   

            // Get translation and apply it (if applicable)
            XMLElement* translateElement = objectElement->FirstChildElement("translate");
            if(translateElement)
            {
                glm::vec3 translate(0.0);
                std::string translateString = translateElement->FirstChild()->Value();
                std::vector<std::string> translatePieces = Utils::parseSpaceSeparatedString(translateString);
                translate.x = (float)std::atof(translatePieces[0].c_str());
                translate.y = (float)std::atof(translatePieces[1].c_str());
                translate.z = (float)std::atof(translatePieces[2].c_str());
                object->translate(translate);
            }
           
            // Get scale and apply it (if applicable)   
            XMLElement* scaleElement = objectElement->FirstChildElement("scale");
            if(scaleElement)
            {
                glm::vec3 scale(1.0);
                std::string scaleString = scaleElement->FirstChild()->Value();
                std::vector<std::string> scalePieces = Utils::parseSpaceSeparatedString(scaleString);
                scale.x = (float)std::atof(scalePieces[0].c_str());
                scale.y = (float)std::atof(scalePieces[1].c_str());
                scale.z = (float)std::atof(scalePieces[2].c_str());
                object->scale(scale);
            }

            // Get rotation and apply it (if applicable)   
            XMLElement* rotateElement = objectElement->FirstChildElement("rotate");
            if(rotateElement)
            {
                glm::vec3 rotateAxis(0.0, 1.0, 0.0);
                float rotateAngle = 0;
                std::string rotateString = rotateElement->FirstChild()->Value();
                std::vector<std::string> rotatePieces = Utils::parseSpaceSeparatedString(rotateString);
                rotateAxis.x = (float)std::atof(rotatePieces[0].c_str());
                rotateAxis.y = (float)std::atof(rotatePieces[1].c_str());
                rotateAxis.z = (float)std::atof(rotatePieces[2].c_str());
                rotateAngle  = (float)std::atof(rotatePieces[3].c_str());
                object->rotate(rotateAxis, rotateAngle); 
            }

            scene->addObject(renderData, object);
        }

        

        // Loop over lights and create them
        for(XMLElement* lightElement = header->FirstChildElement("light"); lightElement != 0; lightElement = lightElement->NextSiblingElement("light"))
        {
            glm::vec3 color;
            std::string colorString = lightElement->FirstChildElement("color")->FirstChild()->Value();
            std::vector<std::string> colorPieces = Utils::parseSpaceSeparatedString(colorString);
            color.x = (float)std::atof(colorPieces[0].c_str());
            color.y = (float)std::atof(colorPieces[1].c_str());
            color.z = (float)std::atof(colorPieces[2].c_str());

            glm::vec3 ambient;
            std::string ambientString = lightElement->FirstChildElement("ambient")->FirstChild()->Value();
            std::vector<std::string> ambientPieces = Utils::parseSpaceSeparatedString(ambientString);
            ambient.x = (float)std::atof(ambientPieces[0].c_str());
            ambient.y = (float)std::atof(ambientPieces[1].c_str());
            ambient.z = (float)std::atof(ambientPieces[2].c_str());

            std::string lightType = lightElement->Attribute("type");
            if(lightType == "directional")
            {
                glm::vec3 direction;
                std::string directionString = lightElement->FirstChildElement("direction")->FirstChild()->Value();
                std::vector<std::string> directionPieces = Utils::parseSpaceSeparatedString(directionString);
                direction.x = (float)std::atof(directionPieces[0].c_str());
                direction.y = (float)std::atof(directionPieces[1].c_str());
                direction.z = (float)std::atof(directionPieces[2].c_str());

                DirectionalLight dirLight;
                dirLight.base.color = color;
                dirLight.base.ambient = ambient;
                dirLight.direction = direction;
                scene->addDirectionalLight(dirLight);
            }
            else if(lightType == "point")
            {
                glm::vec4 position(1.0);
                std::string positionString = lightElement->FirstChildElement("position")->FirstChild()->Value();
                std::vector<std::string> positionPieces = Utils::parseSpaceSeparatedString(positionString);
                position.x = (float)std::atof(positionPieces[0].c_str());
                position.y = (float)std::atof(positionPieces[1].c_str());
                position.z = (float)std::atof(positionPieces[2].c_str());

                PointLight pointLight;
                pointLight.base.color = color;
                pointLight.base.ambient = ambient;
                pointLight.position = position;
                scene->addPointLight(pointLight);
            }
        }

        return scene;
    }
};