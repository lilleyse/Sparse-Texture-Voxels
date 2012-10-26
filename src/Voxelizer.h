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
    float timestamp;
public:

    void begin(VoxelTexture* voxelTexture, CoreEngine* coreEngine, PerFrameUBO* perFrame, GLuint perFrameUBO)
    {
        this->voxelTexture = voxelTexture;
        this->coreEngine = coreEngine;
        this->perFrame = perFrame;
        this->perFrameUBO = perFrameUBO;
        this->timestamp = -1.0f;

        // Create program that writes the scene to a voxel texture
        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "triangleProcessor.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "voxelizer.frag");
        voxelizerProgram = glCreateProgram();
        glAttachShader(voxelizerProgram, vertexShaderObject);
        glAttachShader(voxelizerProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);
        glLinkProgram(voxelizerProgram);
        Utils::OpenGL::checkProgram(voxelizerProgram);
    }

    void voxelizeScene()
    {
        // Update the viewport to be the size of the voxel grid
        int oldViewport[4];
        glGetIntegerv(GL_VIEWPORT, oldViewport);
        uint voxelGridLength = voxelTexture->voxelGridLength;
        glViewport(0, 0, voxelGridLength, voxelGridLength);

        // Disable writing to the framebuffer
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        // Bind voxelTexture's color and normal textures for writing
        glBindImageTexture(COLOR_IMAGE_3D_BINDING_BASE, voxelTexture->colorTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
        glBindImageTexture(NORMAL_IMAGE_3D_BINDING_BASE, voxelTexture->normalTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8_SNORM);
        
        // Use the voxelizer program
        glUseProgram(voxelizerProgram);
		
        // Update the per frame UBO with the orthographic projection
        glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
        timestamp *= -1.0f;
        perFrame->uTimestamp = timestamp;
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
        
        // Memory barrier waits til the 3d texture is completely written before you try to read to the CPU with glGetTexImage
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

        // return values back to normal
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
};