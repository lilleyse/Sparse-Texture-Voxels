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
        glBindImageTexture(COLOR_IMAGE_3D_BINDING_BASE, voxelTexture->colorTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
        glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
        perFrame->uResolution = glm::ivec2(voxelGridLength);
        
        // Render down z-axis
        perFrame->uViewProjection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f)*glm::lookAt(glm::vec3(0,0,0), glm::vec3(0,0,-1), glm::vec3(0,1,0));
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
        coreEngine->display();

        // Render down y-axis
        perFrame->uViewProjection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f)*glm::lookAt(glm::vec3(0,0,0), glm::vec3(0,-1,0), glm::vec3(1,0,0));
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
        coreEngine->display();
        
        // Render down x-axis
        perFrame->uViewProjection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f)*glm::lookAt(glm::vec3(0,0,0), glm::vec3(-1,0,0), glm::vec3(0,0,1));
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
        coreEngine->display();

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
};