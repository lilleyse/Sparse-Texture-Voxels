#pragma once

#include <tinyxml2.h>

#include "../Utils.h"
#include "Buffer.h"
#include "ShaderLibrary.h"
#include "RenderData.h"
#include "MaterialLibrary.h"
#include "Mesh.h"
#include "MeshLoader.h"

using namespace tinyxml2;

struct MeshLibrary
{
    MaterialLibrary materialLibrary;
    std::map<std::string, Mesh*> meshMap;

    MeshLoader<Vertex, unsigned short> staticMeshLoader;
    MeshLoader<Vertex, uint> largeStaticMeshLoader;

    int vertexBufferSize;
    int elementArrayBufferSize;

    
    MeshLibrary() : vertexBufferSize(0), elementArrayBufferSize(0)
    {
        // Nothing
    }

    ~MeshLibrary()
    {
        for(std::map<std::string, Mesh*>::iterator iter = meshMap.begin(); iter != meshMap.end(); iter++)
        {
                delete (*iter).second;
        }
        meshMap.clear();
    }

    void begin()
    {
        // Nothing
    }

    void insert(std::string name, Mesh* mesh)
    {
        meshMap[name] = mesh;
    }

    Mesh* get(std::string name)
    {
        return meshMap[name];
    }

    Mesh* loadMeshFile(std::string& name, std::string& filename)
    {
        // Get information from the mesh file to determine which loader to use

        // Open XML document
        XMLDocument doc;
        doc.LoadFile(filename.c_str());
        if(doc.Error())
                return 0;
        XMLHandle docHandle(&doc);
        XMLElement* header = docHandle.FirstChildElement("Mesh").ToElement();

        // Type is either "static" or "animated"
        std::string headerType = header->FirstChildElement("info")->FirstChildElement("type")->FirstChild()->Value();

        // Get the number of elements
        int numVertices = atoi(header->FirstChildElement("geometry")->FirstChildElement("vertex_data")->Attribute("count"));

        // Load the materials
        for(XMLElement* materialElement = header->FirstChildElement("material"); materialElement != 0; materialElement = materialElement->NextSiblingElement("material"))
        {
            materialLibrary.loadMaterial(materialElement);
        }
    

        Mesh* mesh;
        if(headerType == "static" && numVertices < 65536)
        {
            mesh = staticMeshLoader.loadMeshFile(header, materialLibrary);
        }
        else if(headerType == "static" && numVertices > 65536)
        {
            mesh = largeStaticMeshLoader.loadMeshFile(header, materialLibrary);
        }

        insert(name, mesh);

        // Find this mesh's position in buffers

        // For each LOD in the mesh ...
        for(Mesh* meshLOD = mesh; meshLOD != 0; meshLOD = meshLOD->nextLOD)
        {
            // Update the vertex buffer size
            int vertexBufferStart = Utils::roundToNextMultiple(vertexBufferSize, meshLOD->vertexSize);
            int vertexBufferExtension = meshLOD->numVertices * meshLOD->vertexSize;
            vertexBufferSize = vertexBufferStart + vertexBufferExtension; // update the global variable

            // For each mesh group in the LOD ...
            for(Mesh* meshGroup = meshLOD; meshGroup != 0; meshGroup = meshGroup->nextMeshGroup)
            {
                // Update the element array buffer size
                int elementArrayBufferStart = Utils::roundToNextMultiple(elementArrayBufferSize, meshGroup->elementSize);
                int elementArrayBufferExtension = meshGroup->numElements * meshGroup->elementSize;
                elementArrayBufferSize = elementArrayBufferStart + elementArrayBufferExtension; // update the global variable
                
                // Set the mesh group's baseElement and baseVertex
                meshGroup->baseElement = elementArrayBufferStart/meshGroup->elementSize;
                meshGroup->baseVertex = vertexBufferStart/meshLOD->vertexSize;
            }
        }


        return mesh;
    }

    void commitToGL(RenderData& renderData)
    {

        materialLibrary.commitToGL(renderData);

        // Create the mesh buffer with the known vertex buffer size and element array buffer size
        renderData.comitMeshBuffer(vertexBufferSize, elementArrayBufferSize);
        
        // For each mesh ...
        for(std::map<std::string, Mesh*>::iterator meshIter = meshMap.begin(); meshIter != meshMap.end(); meshIter++)
        {
            Mesh* mesh = meshIter->second;
            for(Mesh* meshLOD = mesh; meshLOD != 0; meshLOD = meshLOD->nextLOD)
            {
                void* vertexData = meshLOD->vertexData;
                uint vertexDataOffset = meshLOD->baseVertex * meshLOD->vertexSize;
                uint vertexDataLength = meshLOD->numVertices * meshLOD->vertexSize;
            
                // Place vertex data into the vertex data buffer
                renderData.commitVertexData(vertexData, vertexDataOffset, vertexDataLength);

                // Go over each render group and add each element array data to the main buffer
                for(Mesh* meshGroup = meshLOD; meshGroup != 0; meshGroup = meshGroup->nextMeshGroup)
                {
                    void* elementArrayData = meshGroup->elementArrayData;
                    uint elementArrayDataOffset = meshGroup->baseElement * meshLOD->elementSize;
                    uint elementArrayDataLength = meshGroup->numElements * meshLOD->elementSize;
                
                    // Place data into the element array buffer
                    renderData.commitElementArrayData(elementArrayData, elementArrayDataOffset, elementArrayDataLength);
                }    
            }
        }
    }

};