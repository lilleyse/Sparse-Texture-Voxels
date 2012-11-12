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
    Camera* viewCamera;
    PerFrameUBO* perFrame;
    GLuint perFrameUBO;
    GLuint voxelizerProgram;
public:

    void begin(VoxelTexture* voxelTexture, CoreEngine* coreEngine, Camera* viewCamera, PerFrameUBO* perFrame, GLuint perFrameUBO)
    {
        this->voxelTexture = voxelTexture;
        this->coreEngine = coreEngine;
        this->viewCamera = viewCamera;
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

        // Bind the six texture directions for writing
        for(uint i = 0; i < voxelTexture->NUM_DIRECTIONS; i++)
            glBindImageTexture(COLOR_IMAGE_POSX_3D_BINDING + i, voxelTexture->colorTextures[i], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
        
        glUseProgram(voxelizerProgram);
        glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);

        float worldSize = perFrame->uVoxelRegionWorld.w;
        float halfSize = worldSize/2.0f;
        float xMin = perFrame->uVoxelRegionWorld.x;
        float yMin = perFrame->uVoxelRegionWorld.y;
        float zMin = perFrame->uVoxelRegionWorld.z;
        float xMax = xMin + worldSize;
        float yMax = yMin + worldSize;
        float zMax = zMin + worldSize;
        float xMid = (xMin + xMax)/2.0f;
        float yMid = (yMin + yMax)/2.0f;
        float zMid = (zMin + zMax)/2.0f;

        glm::mat4 orthoProjection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, worldSize);

        // Render down z-axis
        perFrame->uViewProjection = orthoProjection*glm::lookAt(glm::vec3(xMid,yMid,zMin), glm::vec3(xMid,yMid,zMax), glm::vec3(0,1,0));
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
        coreEngine->display();

        // Render down y-axis
        perFrame->uViewProjection = orthoProjection*glm::lookAt(glm::vec3(xMid,yMin,zMid), glm::vec3(xMid,yMax,zMid), glm::vec3(1,0,0));
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
        coreEngine->display();
        
        // Render down x-axis
        perFrame->uViewProjection = orthoProjection*glm::lookAt(glm::vec3(xMin,yMid,zMid), glm::vec3(xMax,yMid,zMid), glm::vec3(0,0,1));
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
        coreEngine->display();

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
};