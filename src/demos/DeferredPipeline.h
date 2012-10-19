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
    DeferredWrite* deferredWrite;

public:
    void begin(VoxelTexture* voxelTexture, FullScreenQuad* fullScreenQuad, CoreEngine* coreEngine, DeferredWrite* deferredWrite)
    {
        this->voxelTexture = voxelTexture;
        this->fullScreenQuad = fullScreenQuad;
        this->coreEngine = coreEngine;
        this->deferredWrite = deferredWrite;

        // Create program that reads the deferred data
        GLuint vertexShaderObjectRead = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "fullscreen.vert");
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
        // Render to the FBO
        deferredWrite->display();   
        
        // Show the results written to the FBO
        glUseProgram(deferredReadProgram);
        fullScreenQuad->display();
    }
};
