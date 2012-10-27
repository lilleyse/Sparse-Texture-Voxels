#pragma once
#include "Utils.h"
#include "VoxelTexture.h"
#include "ShaderConstants.h"
#include "Passthrough.h"
#include "engine/CoreEngine.h"

class VoxelLighting
{
private:
    VoxelTexture* voxelTexture;
    CoreEngine* coreEngine;
    Passthrough* passthrough;
    PerFrameUBO* perFrame;
    GLuint perFrameUBO;
    GLuint lightingProgram;
public:

    void begin(VoxelTexture* voxelTexture, CoreEngine* coreEngine, Passthrough* passthrough, PerFrameUBO* perFrame, GLuint perFrameUBO)
    {
        this->voxelTexture = voxelTexture;
        this->coreEngine = coreEngine;
        this->passthrough = passthrough;
        this->perFrame = perFrame;
        this->perFrameUBO = perFrameUBO;

        // Create program that writes the scene to a voxel texture
        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "triangleProcessor.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "lighting.frag");
        lightingProgram = glCreateProgram();
        glAttachShader(lightingProgram, vertexShaderObject);
        glAttachShader(lightingProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);
        glLinkProgram(lightingProgram);
        Utils::OpenGL::checkProgram(lightingProgram);
    }

    void lightScene(glm::mat4& lightViewProjectionMatrix)
    {
        // Update the per frame UBO with the orthographic projection
        glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
        perFrame->uViewProjection = lightViewProjectionMatrix;
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Depth pre-pass
        passthrough->passthrough();

        // Bind voxelTexture's color and normal textures for writing
        glBindImageTexture(COLOR_IMAGE_3D_BINDING_BASE, voxelTexture->colorTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindImageTexture(NORMAL_IMAGE_3D_BINDING, voxelTexture->normalTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8_SNORM);
        
        // Use the lighting program
        Utils::OpenGL::setScreenSizedViewport();
        Utils::OpenGL::setRenderState(true, true, false);
        glUseProgram(lightingProgram);
        coreEngine->display();

        // Memory barrier waits til the 3d texture is completely written before you try to read to the CPU with glGetTexImage
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    }
};