#pragma once
#include "Buffer.h"
#include "ShaderLibrary.h"
#include "RenderData.h"
#include "MaterialLibrary.h"
#include <map>
#include <glf.hpp>
#include <tinyxml2.h>
#include "Mesh.h"
#include <istream>
#include <sstream>
#include <iterator>
#include <iostream>
using namespace tinyxml2;


template<class VertexType, class ElementType>
struct MeshLoader
{
    MeshLoader(){}
    ~MeshLoader(){}

    // meshName == NULL if you don't care to reference this mesh by name
    Mesh* createMesh(GLenum drawPrimitive, glm::vec3& extents, std::vector<VertexType>* vertexData, std::vector<ElementType>* elementArrayData, unsigned int materialIndex)
    {
        // Create the Mesh
        unsigned int numVertices = (*vertexData).size();
        unsigned int vertexSize = sizeof(VertexType);
        unsigned int numElements = (*elementArrayData).size();
        unsigned int elementSize = sizeof(ElementType);
        
        Mesh* mesh = new Mesh(&(*vertexData)[0], &(*elementArrayData)[0], extents, drawPrimitive, vertexSize, numVertices, elementSize, numElements, materialIndex);

        return mesh;
    }

    std::vector<VertexType>* parseVertexDataString(const char* dataString, int count, bool containsPositions, bool containsNormals, bool containsUVs)
    {
        std::vector<VertexType>* vertices = new std::vector<VertexType>(count);

        char* string = const_cast<char*>(dataString);
        char* pos = strtok(string, " ");
        int counter = 0;

        while(counter < count)
        {
            if(containsPositions)
            {
                (*vertices)[counter].position.x = (float)std::atof(pos); pos = strtok(NULL, " ");
                (*vertices)[counter].position.y = (float)std::atof(pos); pos = strtok(NULL, " ");
                (*vertices)[counter].position.z = (float)std::atof(pos); pos = strtok(NULL, " ");
            }

            if(containsNormals)
            {
                (*vertices)[counter].normal.x = (float)std::atof(pos); pos = strtok(NULL, " ");
                (*vertices)[counter].normal.y = (float)std::atof(pos); pos = strtok(NULL, " ");
                (*vertices)[counter].normal.z = (float)std::atof(pos); pos = strtok(NULL, " ");
            }

            if(containsUVs)
            {
                (*vertices)[counter].UV.x = (float)std::atof(pos); pos = strtok(NULL, " ");
                (*vertices)[counter].UV.y = (float)std::atof(pos); pos = strtok(NULL, " ");
            }

            /*
            if(animated)
            {
                vertices[counter].animatedStuff = (float)std::atof(pos); pos = strtok(NULL, " ");
            }
            */

            counter++;
        }

        return vertices;
    }

    std::vector<ElementType>* parseElementArrayString(const char* dataString, int count)
    {
        std::vector<ElementType>* elements = new std::vector<ElementType>(count);

        char* string = const_cast<char*>(dataString);
        char* pos = strtok(string, " ");
        int counter = 0;

        while(pos)
        {
            (*elements)[counter] = (ElementType)std::atoi(pos);
            counter++;
            pos = strtok(NULL, " ");
        }

        return elements;
    }

    Mesh* loadMeshFile(XMLElement* meshDoc, MaterialLibrary& materialLibrary)
    {
        XMLElement* infoElement = meshDoc->FirstChildElement("info");

        // Check for the enabled vertex attributes
        std::string truthString = "true";
        bool containsPositions = infoElement->FirstChildElement("positions")->FirstChild()->Value() == truthString;
        bool containsNormals = infoElement->FirstChildElement("normals")->FirstChild()->Value() == truthString;
        bool containsUVs = infoElement->FirstChildElement("UVs")->FirstChild()->Value() == truthString;

        // Get dimensions
        float width = (float)atof(infoElement->FirstChildElement("width")->FirstChild()->Value());
        float height = (float)atof(infoElement->FirstChildElement("height")->FirstChild()->Value());
        float depth = (float)atof(infoElement->FirstChildElement("depth")->FirstChild()->Value());
        glm::vec3 extents(width/2.0f, height/2.0f, depth/2.0f);

        Mesh* mainMesh = 0;
        Mesh* currentMeshLOD = 0;
        Mesh* currentMeshGroup = 0;

        // Loop over the LOD's
        for(XMLElement* geometryElement = meshDoc->FirstChildElement("geometry"); geometryElement != 0; geometryElement = geometryElement->NextSiblingElement("geometry"))
        {

            // Load the vertex data
            XMLElement* vertexDataElement = geometryElement->FirstChildElement("vertex_data");
            int numVertices = atoi(vertexDataElement->Attribute("count"));
            const char* vertexDataString = vertexDataElement->FirstChild()->Value();
            std::vector<VertexType>* vertexData = parseVertexDataString(vertexDataString, numVertices, containsPositions, containsNormals, containsUVs);

            // Loop over the element arrays
            for(XMLElement* elementArrayElement = geometryElement->FirstChildElement("element_array"); elementArrayElement != 0; elementArrayElement = elementArrayElement->NextSiblingElement("element_array"))
            {
                // Load the element array data
                int numElements = atoi(elementArrayElement->Attribute("count"));
                const char* elementArrayString = elementArrayElement->FirstChild()->Value();
                std::vector<ElementType>* elementArrayData = parseElementArrayString(elementArrayString, numElements);
                
                std::string materialName = elementArrayElement->Attribute("material");
                unsigned int materialIndex = materialLibrary.getMaterial(materialName);

                // Create the mesh
                Mesh* meshGroup = createMesh(GL_TRIANGLES, extents, vertexData, elementArrayData, materialIndex);

                // If the first mesh group has not been processed yet, then this mesh must be the first mesh group
                // Else if the first mesh group has already been processed, add this mesh to the first's mesh group list
                if(mainMesh == 0)
                {
                    mainMesh = meshGroup;
                    currentMeshLOD = meshGroup;
                }
                else if(currentMeshGroup == 0)
                {
                    currentMeshLOD->setNextLOD(meshGroup);
                    currentMeshLOD = meshGroup;
                }
                else
                {
                    currentMeshGroup->setNextMeshGroup(meshGroup);
                }

                currentMeshGroup = meshGroup;
            }

            currentMeshGroup = 0;
        }

        // Return the primary mesh
        return mainMesh;
    }
};