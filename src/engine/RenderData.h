#pragma once

#include <CL/opencl.h>

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "Buffer.h"
#include "ShaderLibrary.h"
#include "Object.h"
#include "RenderGroup.h"

class RenderData
{
private:

    std::vector<RenderGroup*> renderGroups;
    std::vector<Object*> objects;
    
    // Buffers that store the materials and positions of all the objects
    UniformBuffer* positionBuffer; // Dynamic GL/CL buffer
    UniformBuffer* materialBuffer; // Static GL buffer

    // Buffer that store per object information
    PerObjectBufferDynamic* perObjectBufferDynamic;
                                
    // Buffer storage that all Meshes share
    MeshBuffer* meshBuffer;

    RenderGroup::MeshMetaData addToRenderGroup(Object* object, Mesh* mesh)
    {
        // Find the render group first
        RenderGroup* myRenderGroup = 0;
        for(uint i = 0; i < renderGroups.size(); i++)
        {
            RenderGroup* renderGroup = renderGroups[i];

            if(renderGroup->isMeshCompatible(object, mesh))
            {
                myRenderGroup = renderGroup;
                break;
            }
        }

        if(myRenderGroup == 0)
        {
            // If no render group could add the object, create a new render group and add the object to it
            uint ID = renderGroups.size();
            myRenderGroup = new RenderGroup(object, mesh, ID);
            renderGroups.push_back(myRenderGroup);
        }

        RenderGroup::MeshMetaData meshMetaData = myRenderGroup->addMesh(object, mesh);
        return meshMetaData;
    }

public:

    void begin()
    {
        // Nothing
    }

    void updateObject(Object* object)
    {
        positionBuffer->commitToGL(&object->position.modelMatrix, sizeof(ObjectPosition), sizeof(ObjectPosition)*object->globalIndex);
    }

    void addObject(Object* object)
    {
        // Push the object into the objects list
        objects.push_back(object);

        RenderGroup::MeshMetaData thisLODMetaData = addToRenderGroup(object, object->mesh);
        RenderGroup::MeshMetaData nextLODMetaData;

        // Increment object count in the renderGroup that hold's the object's primary mesh
        object->renderGroupID = thisLODMetaData.renderGroupID;
        object->drawCommandID = thisLODMetaData.drawCommandIndex;

        // For each LOD of the object's mesh ...
        for(Mesh* meshLOD = object->mesh; meshLOD != 0; meshLOD = meshLOD->nextLOD)
        {

            RenderGroup::MeshMetaData thisMeshGroupMetaData = thisLODMetaData;
            RenderGroup::MeshMetaData nextMeshGroupMetaData;

            // For each mesh group of the LOD ...
            for(Mesh* meshGroup = meshLOD; meshGroup != 0; meshGroup = meshGroup->nextMeshGroup)
            {
                // If this is the last mesh group, make it's next pointers -1
                if(meshGroup->nextMeshGroup == 0)
                {
                    DrawCommand& thisDrawCommand = renderGroups[thisMeshGroupMetaData.renderGroupID]->drawCommands[thisMeshGroupMetaData.drawCommandIndex];
                    thisDrawCommand.renderGroupIDForNextMeshGroup = -1;
                    thisDrawCommand.drawCommandIDForNextMeshGroup = -1;
                }
                else
                {
                    Mesh* nextMeshGroup = meshGroup->nextMeshGroup;
                    nextMeshGroupMetaData = addToRenderGroup(object, nextMeshGroup);

                    DrawCommand& thisDrawCommand = renderGroups[thisMeshGroupMetaData.renderGroupID]->drawCommands[thisMeshGroupMetaData.drawCommandIndex];
                    thisDrawCommand.renderGroupIDForNextMeshGroup = nextMeshGroupMetaData.renderGroupID;
                    thisDrawCommand.drawCommandIDForNextMeshGroup = nextMeshGroupMetaData.drawCommandIndex;

                    thisMeshGroupMetaData = nextMeshGroupMetaData;
                }
            }

            // If this is the last LOD, make it's next pointers -1
            if(meshLOD->nextLOD == 0)
            {
                DrawCommand& thisDrawCommand = renderGroups[thisLODMetaData.renderGroupID]->drawCommands[thisLODMetaData.drawCommandIndex];
                thisDrawCommand.renderGroupIDForNextLOD = -1;
                thisDrawCommand.drawCommandIDForNextLOD = -1;
            }
            else
            {
                Mesh* nextLOD = meshLOD->nextLOD;
                nextLODMetaData = addToRenderGroup(object, nextLOD);

                DrawCommand& thisDrawCommand = renderGroups[thisLODMetaData.renderGroupID]->drawCommands[thisLODMetaData.drawCommandIndex];
                thisDrawCommand.renderGroupIDForNextLOD = nextLODMetaData.renderGroupID;
                thisDrawCommand.drawCommandIDForNextLOD = nextLODMetaData.drawCommandIndex;

                thisLODMetaData = nextLODMetaData;
            }
        }
    }

    void commitMaterials(void* materialData, uint subBufferSize, uint bufferSize)
    {
        materialBuffer = new UniformBuffer(MESH_MATERIAL_ARRAY_BINDING, 0, bufferSize, GL_STATIC_DRAW);
        materialBuffer->commitToGL(materialData, subBufferSize, 0);
    }
   
    void comitMeshBuffer(uint vertexBufferSize, uint elementArraySize)
    {
        meshBuffer = new MeshBuffer(vertexBufferSize, elementArraySize, GL_STATIC_DRAW);
    }

    void commitVertexData(void* vertexData, uint offset, uint size)
    {
        meshBuffer->subVertexBuffer(vertexData, offset, size);
    }

    void commitElementArrayData(void* elementArrayData, uint offset, uint size)
    {
        meshBuffer->subElementArrayBuffer(elementArrayData, offset, size);
    }

    void commitToGL()
    {
        uint meshCount = 0;
        uint globalBaseInstance = 0;

        // For each render group ...
        for(uint i = 0; i < renderGroups.size(); i++)
        {
            RenderGroup* renderGroup = renderGroups[i];
            meshCount += renderGroup->meshCount;

            // Set the base instances
            for(uint j = 0; j < renderGroup->drawCommands.size(); j++)
            {
                DrawCommand& drawCommand = renderGroup->drawCommands[j];
                drawCommand.baseInstance = globalBaseInstance;
                globalBaseInstance += drawCommand.primCount;
                drawCommand.primCount = 0;
            }
        }


        // Now that the number of objects in the scene is known, create the vectors that store object stuff
        std::vector<glm::ivec2>perObjectArrayDynamic(meshCount);
        std::vector<ObjectPosition> positionArray(objects.size());


        // For every object ...
        for(uint i = 0; i < objects.size(); i++)
        {
            Object* object = objects[i];
            
            int objectIndex = i;
            object->globalIndex = objectIndex;
            positionArray[objectIndex] = object->position;

            // Loop over all the mesh groups of the first LOD

            int meshGroupRenderGroup = object->renderGroupID;
            int meshGroupDrawCommand = object->drawCommandID;
            while(meshGroupRenderGroup != -1 && meshGroupDrawCommand != -1)
            {
                DrawCommand& drawCommand = renderGroups[meshGroupRenderGroup]->drawCommands[meshGroupDrawCommand];
                meshGroupRenderGroup = drawCommand.renderGroupIDForNextMeshGroup;
                meshGroupDrawCommand = drawCommand.drawCommandIDForNextMeshGroup;

                uint baseInstance = drawCommand.baseInstance;
                uint instanceCount = drawCommand.primCount++;
                uint globalIndex = baseInstance + instanceCount;
                uint materialOffset = drawCommand.materialOffset;

                glm::ivec2 perObjectDynamic;
                perObjectDynamic[POSITION_INDEX] = objectIndex;
                perObjectDynamic[MATERIAL_INDEX] = materialOffset;
                perObjectArrayDynamic[globalIndex] = perObjectDynamic;
            }
        }


        // Create more buffers now that all objects have been processed
        positionBuffer = new UniformBuffer(POSITION_ARRAY_BINDING, 0, sizeof(ObjectPosition)*NUM_OBJECTS_MAX, GL_STATIC_DRAW); // TO-DO: change to stream once things start moving
        positionBuffer->commitToGL(&positionArray[0], sizeof(ObjectPosition)*positionArray.size(), 0);
        
        perObjectBufferDynamic = new PerObjectBufferDynamic(&perObjectArrayDynamic[0], sizeof(glm::ivec2)*perObjectArrayDynamic.size());

        // For each render group ...
        for(uint i = 0; i < renderGroups.size(); i++)
        {
            RenderGroup* renderGroup = renderGroups[i];        
            
            // Create the vao for this render group
            renderGroup->createVAO(meshBuffer, perObjectBufferDynamic);
        }
    }


    void display()
    {
        for(uint i = 0; i < renderGroups.size(); i++)
        {
            RenderGroup* renderGroup = renderGroups[i];
            if(!renderGroup->disabled)
            {
                renderGroup->render(true);
            }
        }
    }

    ~RenderData()
    {

    }

};
