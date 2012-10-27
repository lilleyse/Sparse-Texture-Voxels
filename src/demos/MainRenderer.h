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

        // Create shader program
        std::string vertexShaderSource = SHADER_DIRECTORY + "triangleProcessor.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "mainRendererDemo.frag";
        mainRendererProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
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
