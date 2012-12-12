#pragma once
#include "Utils.h"
#include "VoxelTexture.h"
#include "ShaderConstants.h"
#include "engine/CoreEngine.h"

class Voxelizer
{
private:
    VoxelTexture* voxelTexture;
    CoreEngine* coreEngine;
    PerFrameUBO* perFrame;

    GLuint perFrameUBO;
    GLuint voxelizerProgram;
public:

    void begin(VoxelTexture* voxelTexture, CoreEngine* coreEngine, PerFrameUBO* perFrame, GLuint perFrameUBO)
    {
        this->voxelTexture = voxelTexture;
        this->coreEngine = coreEngine;
        this->perFrame = perFrame;
        this->perFrameUBO = perFrameUBO;

        // Create shader program
        std::string vertexShaderSource = SHADER_DIRECTORY + "triangleProcessor.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "voxelizer.frag";
        voxelizerProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }

    void voxelizeScene()
    {
        uint voxelGridLength = voxelTexture->voxelGridLength;
        Utils::OpenGL::setViewport(voxelGridLength, voxelGridLength);
        Utils::OpenGL::setRenderState(false, false, false);

        glUseProgram(voxelizerProgram);
        glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);

        for(uint i = 0; i < voxelTexture->numCascades; i++)
        {
            perFrame->uCurrentCascade = i;

            // Bind the six texture directions for writing
            for(uint j = 0; j < NUM_VOXEL_DIRECTIONS; j++)
                glBindImageTexture(VOXEL_DIRECTIONS_IMAGE_ARRAY_BINDING[j], voxelTexture->colorTextures[i*NUM_VOXEL_DIRECTIONS + j], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

            float worldSize = perFrame->uVoxelRegionWorld[i].w;
            float halfSize = worldSize/2.0f;
            float voxelSize = worldSize/perFrame->uVoxelRes;
            glm::vec3 bMid = glm::vec3(perFrame->uVoxelRegionWorld[i]);
            glm::vec3 bMin = bMid - halfSize;
            glm::vec3 bMax = bMid + halfSize;
            
            
            glm::mat4 orthoProjection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, worldSize);

            // Render down z-axis
            perFrame->uViewProjection = orthoProjection*glm::lookAt(glm::vec3(bMid.x,bMid.y,bMin.z), glm::vec3(bMid.x,bMid.y,bMax.z), glm::vec3(0,1,0));
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
            coreEngine->display();

            // Render down y-axis
            perFrame->uViewProjection = orthoProjection*glm::lookAt(glm::vec3(bMid.x,bMin.y,bMid.z), glm::vec3(bMid.x,bMax.y,bMid.z), glm::vec3(1,0,0));
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
            coreEngine->display();
        
            // Render down x-axis
            perFrame->uViewProjection = orthoProjection*glm::lookAt(glm::vec3(bMin.x,bMid.y,bMid.z), glm::vec3(bMax.x,bMid.y,bMid.z), glm::vec3(0,0,1));
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
            coreEngine->display();
        }
        
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
};