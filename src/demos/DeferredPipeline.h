#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "../FullScreenQuad.h"
#include "../VoxelTexture.h"
#include "../DeferredWrite.h"
#include "../engine/CoreEngine.h"

class DeferredPipeline
{
private:

    GLuint deferredReadProgram;

    VoxelTexture* voxelTexture;
    FullScreenQuad* fullScreenQuad;
    CoreEngine* coreEngine;

public:
    void begin(VoxelTexture* voxelTexture, FullScreenQuad* fullScreenQuad, CoreEngine* coreEngine)
    {
        this->voxelTexture = voxelTexture;
        this->fullScreenQuad = fullScreenQuad;
        this->coreEngine = coreEngine;

        // Create program that reads the deferred data
        GLuint vertexShaderObjectRead = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "mainDeferred.vert");
        GLuint fragmentShaderObjectRead = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "deferredRead.frag");

        deferredReadProgram = glCreateProgram();
        glAttachShader(deferredReadProgram, vertexShaderObjectRead);
        glAttachShader(deferredReadProgram, fragmentShaderObjectRead);
        glDeleteShader(vertexShaderObjectRead);
        glDeleteShader(fragmentShaderObjectRead);

        glLinkProgram(deferredReadProgram);
        Utils::OpenGL::checkProgram(deferredReadProgram);
    }

    void display()
    {
        //voxelTexture->enableNearestSampling();
        voxelTexture->enableLinearSampling();
        
        // rasterize triangles and render colors
        glUseProgram(deferredReadProgram);
        
        glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
        coreEngine->display();
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        coreEngine->display();
    }
};
