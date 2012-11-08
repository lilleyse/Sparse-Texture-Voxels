#pragma once

#include "../Utils.h"
#include "../VoxelTexture.h"

class VoxelDebug
{
private:
    int currentMipMapLevel;
    GLuint vertexArray;
    GLuint voxelBuffer;
    GLuint voxelDebugProgram;
    static const uint numVerticesCube = 24;
    static const uint numElementsCube = 36; 
    VoxelTexture* voxelTexture;
    std::vector<MipMapInfo> debugMipMapInfoArray;

    struct Voxel
    {
        // .xyz are translation, .w is scale
        glm::vec4 transformation;
        glm::vec4 color;
    };

public:
    VoxelDebug(){}
    virtual ~VoxelDebug(){}

    void begin(VoxelTexture* voxelTexture)
    {
        this->currentMipMapLevel = 0;
        this->voxelTexture = voxelTexture;

        // Create buffer objects and vao
        glm::vec3 vertices[numVerticesCube*2] = {glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-1, 0, -0), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-1, 0, -0), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(-1, 0, -0), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-1, 0, -0), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0, 0, -1), glm::vec3(0.5, 0.5, -0.5), glm::vec3(0, 0, -1), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0, 0, -1), glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0, 0, -1), glm::vec3(0.5, 0.5, -0.5), glm::vec3(1, 0, -0), glm::vec3(0.5, 0.5, 0.5), glm::vec3(1, 0, -0), glm::vec3(0.5, -0.5, 0.5), glm::vec3(1, 0, -0), glm::vec3(0.5, -0.5, -0.5), glm::vec3(1, 0, -0), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0, 0, 1), glm::vec3(0.5, -0.5, 0.5), glm::vec3(0, 0, 1), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0, 0, 1), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0, 0, 1), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0, -1, -0), glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0, -1, -0), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0, -1, -0), glm::vec3(0.5, -0.5, 0.5), glm::vec3(0, -1, -0), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0, 1, -0), glm::vec3(0.5, 0.5, -0.5), glm::vec3(0, 1, -0), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0, 1, -0), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0, 1, -0)};
        unsigned short elements[numElementsCube] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};
        
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*2*numVerticesCube, vertices, GL_STATIC_DRAW);

        GLuint elementArrayBuffer;
        glGenBuffers(1, &elementArrayBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*numElementsCube, elements, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenBuffers(1, &voxelBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        // Don't fill this buffer yet
        
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(POSITION_ATTR);
        glEnableVertexAttribArray(NORMAL_ATTR);
        glVertexAttribPointer(POSITION_ATTR, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3)*2, 0);
        glVertexAttribPointer(NORMAL_ATTR, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3)*2, (void*)sizeof(glm::vec3));

        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        glEnableVertexAttribArray(DEBUG_TRANSFORM_ATTR);
        glEnableVertexAttribArray(DEBUG_COLOR_ATTR);
        glVertexAttribPointer(DEBUG_TRANSFORM_ATTR, 4, GL_FLOAT, GL_FALSE, sizeof(Voxel), (void*)(0));
        glVertexAttribPointer(DEBUG_COLOR_ATTR, 4, GL_FLOAT, GL_FALSE, sizeof(Voxel), (void*)(sizeof(glm::vec4)));
        glVertexAttribDivisor(DEBUG_TRANSFORM_ATTR, 1);
        glVertexAttribDivisor(DEBUG_COLOR_ATTR, 1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
        
        //Unbind everything
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Create shader program
        std::string vertexShaderSource = SHADER_DIRECTORY + "debugVoxelDemo.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "debugVoxelDemo.frag";
        voxelDebugProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
        
        voxelTextureUpdate();
    }
    void voxelTextureUpdate()
    {
        debugMipMapInfoArray.clear();
        std::vector<Voxel> voxelArray;

        glBindTexture(GL_TEXTURE_3D, voxelTexture->colorTextures[voxelTexture->NEGX]);
        
        float voxelScale = 1.0f / voxelTexture->mipMapInfoArray[0].gridLength;
        for(uint i = 0; i < voxelTexture->numMipMapLevels; i++)
        {
            MipMapInfo debugMipMapInfo;
            debugMipMapInfo.offset = voxelArray.size();

            uint mipMapGridLength = voxelTexture->mipMapInfoArray[i].gridLength;
            std::vector<glm::u8vec4> imageData(mipMapGridLength*mipMapGridLength*mipMapGridLength);
            glGetTexImage(GL_TEXTURE_3D, i, GL_RGBA, GL_UNSIGNED_BYTE, &imageData[0]);

            // apply an offset to the position because the origin of the cube model is in its center rather than a corner
            glm::vec3 offset = glm::vec3(voxelScale/2);

            uint textureIndex = 0;
            for(uint j = 0; j < mipMapGridLength; j++)
            for(uint k = 0; k < mipMapGridLength; k++)
            for(uint l = 0; l < mipMapGridLength; l++)
            {
                glm::vec3 position = glm::vec3(l*voxelScale,k*voxelScale,j*voxelScale) + offset;
                glm::u8vec4 color = imageData[textureIndex];
                if(color.a > 0)
                {
                    Voxel voxel;
                    voxel.color = glm::vec4(color)/255.0f;
                    voxel.transformation = glm::vec4(position, voxelScale);
                    voxelArray.push_back(voxel);
                }
                textureIndex++;
            }
            debugMipMapInfo.numVoxels = voxelArray.size() - debugMipMapInfo.offset;
            debugMipMapInfoArray.push_back(debugMipMapInfo);
            voxelScale *= 2;
        }

        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Voxel)*voxelArray.size(), &voxelArray[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void display()
    {
        uint baseInstance = debugMipMapInfoArray[this->currentMipMapLevel].offset;
        uint primCount = debugMipMapInfoArray[this->currentMipMapLevel].numVoxels;

        Utils::OpenGL::setScreenSizedViewport();
        Utils::OpenGL::setRenderState(true, true, true);
        glUseProgram(voxelDebugProgram);
        glBindVertexArray(vertexArray);
        glDrawElementsInstancedBaseInstance(GL_TRIANGLES, numElementsCube, GL_UNSIGNED_SHORT, 0, primCount, baseInstance);
    }

    void setMipMapLevel(uint mipMapLevel)
    {
        this->currentMipMapLevel = mipMapLevel;
    }
};