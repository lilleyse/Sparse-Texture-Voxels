#pragma once

#include "../Utils.h"
#include "../VoxelTexture.h"

struct VoxelInfo
{
    glm::vec4 transformation;
    glm::vec3 sideColors[6];
};

class VoxelDebug
{
private:
    GLuint vertexArray;
    GLuint voxelBuffer;
    GLuint voxelDebugProgram;
    static const uint numVerticesCube = 24;
    static const uint numElementsCube = 36;
    
    std::vector<MipMapInfo> debugMipMapInfoArray;
    VoxelTexture* voxelTexture;
    PerFrameUBO* perFrame;

public:
    VoxelDebug(){}
    virtual ~VoxelDebug(){}

    void begin(VoxelTexture* voxelTexture, PerFrameUBO* perFrame)
    {
        this->voxelTexture = voxelTexture;
        this->perFrame = perFrame;

        this->debugMipMapInfoArray.resize(voxelTexture->numMipMapLevels);

        // Create buffer objects and vao
        glm::vec3 vertices[numVerticesCube] = 
        {
            glm::vec3(0.5, 0.5, -0.5), 
            glm::vec3(0.5, 0.5, 0.5), 
            glm::vec3(0.5, -0.5, 0.5), 
            glm::vec3(0.5, -0.5, -0.5), 
            glm::vec3(-0.5, -0.5, -0.5), 
            glm::vec3(-0.5, -0.5, 0.5), 
            glm::vec3(-0.5, 0.5, 0.5), 
            glm::vec3(-0.5, 0.5, -0.5), 
            glm::vec3(0.5, 0.5, 0.5), 
            glm::vec3(0.5, 0.5, -0.5), 
            glm::vec3(-0.5, 0.5, -0.5), 
            glm::vec3(-0.5, 0.5, 0.5),
            glm::vec3(-0.5, -0.5, 0.5), 
            glm::vec3(-0.5, -0.5, -0.5), 
            glm::vec3(0.5, -0.5, -0.5), 
            glm::vec3(0.5, -0.5, 0.5), 
            glm::vec3(-0.5, -0.5, 0.5), 
            glm::vec3(0.5, -0.5, 0.5), 
            glm::vec3(0.5, 0.5, 0.5), 
            glm::vec3(-0.5, 0.5, 0.5), 
            glm::vec3(-0.5, 0.5, -0.5), 
            glm::vec3(0.5, 0.5, -0.5), 
            glm::vec3(0.5, -0.5, -0.5), 
            glm::vec3(-0.5, -0.5, -0.5)
        };
        
        ushort elements[numElementsCube] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};
        
        // Positions
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*numVerticesCube, vertices, GL_STATIC_DRAW);

        GLuint elementArrayBuffer;
        glGenBuffers(1, &elementArrayBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*numElementsCube, elements, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Per instance transformations
        glGenBuffers(1, &voxelBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        // Don't fill this buffer yet
        
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(POSITION_ATTR);
        glVertexAttribPointer(POSITION_ATTR, 3, GL_FLOAT, GL_FALSE, 0, 0);

        int voxelBufferOffset = 0;
        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        glEnableVertexAttribArray(DEBUG_TRANSFORM_ATTR);
        glVertexAttribPointer(DEBUG_TRANSFORM_ATTR, 4, GL_FLOAT, GL_FALSE, sizeof(VoxelInfo), (void*)voxelBufferOffset);
        glVertexAttribDivisor(DEBUG_TRANSFORM_ATTR, 1);
        voxelBufferOffset += sizeof(glm::vec4);

        for(uint i = 0; i < 6; i++)
        {
            glEnableVertexAttribArray(DEBUG_COLOR_ATTR[i]);
            glVertexAttribPointer(DEBUG_COLOR_ATTR[i], 3, GL_FLOAT, GL_FALSE, sizeof(VoxelInfo), (void*)voxelBufferOffset);
            glVertexAttribDivisor(DEBUG_COLOR_ATTR[i], 1);
            voxelBufferOffset += sizeof(glm::vec3);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
        
        //Unbind everything
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Create shader program
        std::string vertexShaderSource = SHADER_DIRECTORY + "debugVoxelDemo.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "debugVoxelDemo.frag";
        voxelDebugProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }
    void voxelTextureUpdate()
    {
        // Refresh the debug mip map info array
        for(unsigned int i = 0; i < voxelTexture->numMipMapLevels; i++)
        {
            debugMipMapInfoArray[i].numVoxels = 0;
            debugMipMapInfoArray[i].offset = 0;
        }

        std::vector<VoxelInfo> voxels(voxelTexture->totalVoxels);
        std::vector<bool> residency(voxelTexture->totalVoxels, false);

        uint voxelGridLength = voxelTexture->voxelGridLength;
        std::vector<glm::u8vec4> textureData(voxelGridLength*voxelGridLength*voxelGridLength);

        glActiveTexture(GL_TEXTURE0 + NON_USED_TEXTURE);

        // only show the first cascade
        for(uint i = 0; i < NUM_VOXEL_DIRECTIONS; i++)
        {
            glBindTexture(GL_TEXTURE_3D, voxelTexture->colorTextures[i]);
                
            float voxelScale = this->perFrame->uVoxelRegionWorld[i].w / voxelTexture->mipMapInfoArray[0].gridLength;
            for(uint j = 0; j < voxelTexture->numMipMapLevels; j++)
            {
                glGetTexImage(GL_TEXTURE_3D, j, GL_RGBA, GL_UNSIGNED_BYTE, &textureData[0]);

                // apply an offset to the position because the origin of the cube model is in its center rather than a corner
                glm::vec3 offset = glm::vec3(voxelScale/2) + glm::vec3(perFrame->uVoxelRegionWorld[i]) - glm::vec3(perFrame->uVoxelRegionWorld[i].w/2.0);

                uint mipMapGridLength = voxelTexture->mipMapInfoArray[j].gridLength;
                uint textureIndex = 0;
                for(uint x = 0; x < mipMapGridLength; x++)
                for(uint y = 0; y < mipMapGridLength; y++)
                for(uint z = 0; z < mipMapGridLength; z++)
                {
                    glm::vec3 position = glm::vec3(z*voxelScale,y*voxelScale,x*voxelScale) + offset;
                    glm::u8vec4 color = textureData[textureIndex];
                    if(color.a > 0)
                    {
                        uint globalIndex = voxelTexture->mipMapInfoArray[j].offset + textureIndex;
                        voxels[globalIndex].transformation = glm::vec4(position, voxelScale);
                        voxels[globalIndex].sideColors[i] = glm::vec3(color)/255.0f;

                        if(residency[globalIndex] == false)
                        {
                            debugMipMapInfoArray[j].numVoxels++;
                        }
                        
                        residency[globalIndex] = true;
                    }
                    textureIndex++;
                }
                voxelScale *= 2;
            }
        }

        uint numVoxels = 0;
        for(unsigned int i = 0; i < voxelTexture->numMipMapLevels; i++)
        {
            debugMipMapInfoArray[i].offset = numVoxels;
            numVoxels += debugMipMapInfoArray[i].numVoxels;
        }

        std::vector<VoxelInfo> packed(numVoxels);
        int packedIndex = 0;
        for(uint i = 0; i < voxelTexture->totalVoxels; i++)
        {
            if(residency[i] == true)
            {
                packed[packedIndex] = voxels[i];
                packedIndex++;
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VoxelInfo)*packed.size(), &packed[0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void display()
    {
        uint baseInstance = debugMipMapInfoArray[perFrame->uCurrentMipLevel].offset;
        uint primCount = debugMipMapInfoArray[perFrame->uCurrentMipLevel].numVoxels;

        Utils::OpenGL::setScreenSizedViewport();
        Utils::OpenGL::setRenderState(true, true, true);

        glUseProgram(voxelDebugProgram);
        glBindVertexArray(vertexArray);
        glDrawElementsInstancedBaseInstance(GL_TRIANGLES, numElementsCube, GL_UNSIGNED_SHORT, 0, primCount, baseInstance);
    }
};