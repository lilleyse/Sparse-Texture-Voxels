#pragma once
#include "Utils.h"
#include "engine/CoreEngine.h"

class Passthrough
{
private:
    CoreEngine* coreEngine;
    GLuint passthroughProgram;
public:

    void begin(CoreEngine* coreEngine)
    {
        this->coreEngine = coreEngine;

        // Create program that does a simple passthrough
        std::string vertexShaderSource = SHADER_DIRECTORY + "passthrough.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "passthrough.frag";
        passthroughProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }

    void passthrough()
    {        
        // Do not write to the color buffer
        Utils::OpenGL::setScreenSizedViewport();
        Utils::OpenGL::setRenderState(true, true, false);
        glUseProgram(passthroughProgram);
        coreEngine->display();
    }
};