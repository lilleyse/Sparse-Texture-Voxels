#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "../FullScreenQuad.h"
#include "../VoxelTexture.h"
#include "../Passthrough.h"
#include "../engine/CoreEngine.h"

class MainRenderer
{
private:

    GLuint mainRendererProgram;
    CoreEngine* coreEngine;
    Passthrough* passthrough;

public:
    void begin(CoreEngine* coreEngine, Passthrough* passthrough)
    {
        this->coreEngine = coreEngine;
        this->passthrough = passthrough;

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
        // Depth pre-pass
        passthrough->passthrough();
        Utils::OpenGL::setScreenSizedViewport();
        Utils::OpenGL::setRenderState(true, true, true);
        glUseProgram(mainRendererProgram);
        coreEngine->display();
    }
};
