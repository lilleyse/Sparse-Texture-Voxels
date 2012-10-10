#pragma once

#include "../Utils.h"
#include "Object.h"

struct PerObjectBufferDynamic
{
    GLuint bufferObject;

    PerObjectBufferDynamic(void* data, int bufferSize)
    {
        glGenBuffers(1, &bufferObject);
        glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, data, GL_STREAM_COPY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    ~PerObjectBufferDynamic()
    {
        glDeleteBuffers(1, &bufferObject);
    }
};


// Wrapper for OpenGL uniform buffer objects
struct UniformBuffer
{
    GLuint bufferObject;

    UniformBuffer(GLuint bindingIndex, void* data, int bufferSize, GLenum usageType)
    {
        glGenBuffers(1, &bufferObject);
        glBindBuffer(GL_UNIFORM_BUFFER, bufferObject);
        glBufferData(GL_UNIFORM_BUFFER, bufferSize, data, usageType);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, bufferObject);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    ~UniformBuffer()
    {
        glDeleteBuffers(1, &bufferObject);
    }

    void commitToGL(void* data, int size, int offset)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, bufferObject);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
};

// Stores an array buffer for vertices and an element array buffer for indices. Multiple Meshes are packed into the same buffers.
struct MeshBuffer
{
    GLuint vertexBufferObject;
    GLuint elementArrayBufferObject;

    MeshBuffer(uint vertexBufferSize, uint elementArrayBufferSize, GLenum usageType)
    {

        glGenBuffers(1, &vertexBufferObject);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
        glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, usageType);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &elementArrayBufferObject);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBufferObject);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementArrayBufferSize, NULL, usageType);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void subVertexBuffer(void* vertexData, uint vertexDataOffset, uint vertexDataLength)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
        glBufferSubData(GL_ARRAY_BUFFER, vertexDataOffset, vertexDataLength, vertexData);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void subElementArrayBuffer(void* elementArrayData, uint elementArrayDataOffset, uint elementArrayDataLength)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBufferObject);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, elementArrayDataOffset, elementArrayDataLength, elementArrayData);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    ~MeshBuffer()
    {
        glDeleteBuffers(1, &vertexBufferObject);
        glDeleteBuffers(1, &elementArrayBufferObject);
    }

    void commitToGL(int baseVertex, int numVertices, void* vertexData)
    {
        /*
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
        glBufferSubData(GL_ARRAY_BUFFER, baseVertex*vertexSize, numVertices*vertexSize, vertexData);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        */
    }
};