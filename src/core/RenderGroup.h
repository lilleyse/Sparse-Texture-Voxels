#pragma once
#include "ShaderLibrary.h"
#include "Buffer.h"
#include "../Utils.h"

struct DrawElements
{
    
};


struct DrawCommand
{
    // Draw elements
    GLuint count;
    GLuint primCount; // number of instances
    GLuint firstIndex;
    GLint baseVertex;
    GLuint baseInstance; // the count of how many instances there are prior to this, even of different mesh types

    // Helper things
    GLint drawCommandIDForNextLOD; // -1 means no next LOD
    GLint drawCommandIDForNextMeshGroup; // -1 means no next mesh group
    GLint renderGroupIDForNextLOD; // -1 means no next LOD
    GLint renderGroupIDForNextMeshGroup; // -1 means no next mesh group
    GLuint materialOffset;
};


struct RenderGroup
{
    // Unique identifier for this render group
    unsigned int ID;

    // Draw commands
    std::vector<DrawCommand> drawCommands;

    int meshCount; // Updates in addObject
        
    // Properties that distinguish this render group
    int vertexSize;
    int elementSize;
    GLenum elementType;
    GLenum drawPrimitive;
    GLuint shader;

    // Vertex array object built from the render group's properties
    GLuint vertexArrayObject;

    bool disabled;

    RenderGroup(Object* object, Mesh* mesh, unsigned int ID)
    :
        vertexSize(mesh->vertexSize),
        elementSize(mesh->elementSize),
        elementType(mesh->elementType),
        drawPrimitive(mesh->drawPrimitive),
        shader(object->shader),
        ID(ID),
        disabled(false),
        meshCount(0)
    {
        // Nothing
    }
    ~RenderGroup()
    {
        // Nothing
    }


    struct MeshMetaData
    {
        unsigned int renderGroupID;
        unsigned int drawCommandIndex;
    };

    // Returns the draw index that the mesh is placed into
    MeshMetaData addMesh(Object* object, Mesh* mesh)
    {
        meshCount++;

        // Now find the draw command inside the render group
        int drawCommandIndex = -1;

        // Loop over the draw commands
        for(unsigned int i = 0; i < drawCommands.size(); i++)
        {
            DrawCommand& drawCommand = drawCommands[i];
            // If the draw ommand matches this mesh ...
            if(drawCommand.baseVertex == mesh->baseVertex && drawCommand.firstIndex == mesh->baseElement)
            {
                // Increment the instance count
                drawCommand.primCount ++;

                drawCommandIndex = i;
                break;
            }
        }

        if(drawCommandIndex == -1)
        {
            // If no draw command match was found, add a new one
            DrawCommand drawCommand;
            drawCommand.count = mesh->numElements;
            drawCommand.primCount = 1;
            drawCommand.firstIndex = mesh->baseElement;
            drawCommand.baseVertex = mesh->baseVertex;
            drawCommand.baseInstance = 0; // Updated later

            drawCommand.materialOffset = mesh->materialIndex;

            drawCommandIndex = drawCommands.size();
            drawCommands.push_back(drawCommand);
        }

        MeshMetaData meshMetaData;
        meshMetaData.drawCommandIndex = drawCommandIndex;
        meshMetaData.renderGroupID = ID;


        return meshMetaData;
    }
    


    void createVAO(MeshBuffer* meshBuffer, PerObjectBufferDynamic* perObjectBufferDynamic)
    {
        //TO-DO: enable the attributes that are actually used


        // Generate the vertex array object
        glGenVertexArrays(1, &vertexArrayObject);
        glBindVertexArray(vertexArrayObject);

        // Add the array buffer to the state of the vertex array object
        glBindBuffer(GL_ARRAY_BUFFER, meshBuffer->vertexBufferObject);

        // Enable vertex attributes (TO-DO: this will need to accommodate texture coordinates, animation data, and other things in the future)
        glEnableVertexAttribArray(POSITION_ATTR);
        glEnableVertexAttribArray(NORMAL_ATTR);
        glEnableVertexAttribArray(UV_ATTR);
        glEnableVertexAttribArray(PROPERTY_INDEX_ATTR);
        

        // Set the vertex attribute pointers. The vertex attributes are interleaved
        int offset = 0;
        glVertexAttribPointer(POSITION_ATTR, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(offset));
        offset += 3*sizeof(float);
        glVertexAttribPointer(NORMAL_ATTR, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(offset));
        offset += 3*sizeof(float);
        glVertexAttribPointer(UV_ATTR, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(offset));
        offset += 2*sizeof(float);

        // Bind and point to the object index buffer
        glBindBuffer(GL_ARRAY_BUFFER, perObjectBufferDynamic->bufferObject);
        glVertexAttribIPointer(PROPERTY_INDEX_ATTR, 2, GL_INT, 0, 0);
        glVertexAttribDivisor(PROPERTY_INDEX_ATTR, 1);

        // Add the element array buffer to the state of the vertex array object
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer->elementArrayBufferObject);

        //Unbind everything
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void render(bool shaderOverride)
    {
        if(!shaderOverride)
        {
            glUseProgram(shader);
        }

        glBindVertexArray(vertexArrayObject);

        for(uint i = 0; i < drawCommands.size(); i++)
        {
            DrawCommand& drawCommand = drawCommands[i];
            glDrawElementsInstancedBaseVertexBaseInstance(drawPrimitive, drawCommand.count, elementType, (void*)(drawCommand.firstIndex*elementSize), drawCommand.primCount, drawCommand.baseVertex, drawCommand.baseInstance);
        }
    }
        
    bool isMeshCompatible(Object* object, Mesh* mesh)
    {
        return  elementSize == mesh->elementSize &&
                vertexSize == mesh->vertexSize &&
                drawPrimitive == mesh->drawPrimitive &&
                shader == object->shader;
    }

};