#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "../FullScreenQuad.h"
#include "../VoxelTexture.h"
#include "../engine/CoreEngine.h"

class MainRenderer
{
private:

    GLuint mainRendererProgram;

    VoxelTexture* voxelTexture;
    FullScreenQuad* fullScreenQuad;
    CoreEngine* coreEngine;

public:
    void begin(VoxelTexture* voxelTexture, FullScreenQuad* fullScreenQuad, CoreEngine* coreEngine)
    {
        this->voxelTexture = voxelTexture;
        this->fullScreenQuad = fullScreenQuad;
        this->coreEngine = coreEngine;

        GLuint vertexShaderObjectRead = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "triangleProcessor.vert");
        GLuint fragmentShaderObjectRead = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "mainRendererDemo.frag");

        mainRendererProgram = glCreateProgram();
        glAttachShader(mainRendererProgram, vertexShaderObjectRead);
        glAttachShader(mainRendererProgram, fragmentShaderObjectRead);
        glDeleteShader(vertexShaderObjectRead);
        glDeleteShader(fragmentShaderObjectRead);

        glLinkProgram(mainRendererProgram);
        Utils::OpenGL::checkProgram(mainRendererProgram);
    }

    void display()
    { 
        glUseProgram(mainRendererProgram);
        
        //Writes the depth buffer and presumably skips the fragment shader, thus acting like a depth prepass
        glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
        coreEngine->display();

        //Render again but this time uses the frament shader and skips occluded fragments 
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        coreEngine->display();
    }
};
