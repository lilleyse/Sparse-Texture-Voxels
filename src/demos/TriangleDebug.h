#pragma once
#include "../Utils.h"
#include "../engine/CoreEngine.h"

class TriangleDebug
{
private:

    GLuint triangleDebugProgram;
    CoreEngine* coreEngine;

public:

    void begin(CoreEngine* coreEngine)
    {
        this->coreEngine = coreEngine;

        // Create shader program
        std::string vertexShaderSource = SHADER_DIRECTORY + "triangleProcessor.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "triangleDebugDemo.frag";
        triangleDebugProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }

    void display()
    {
        Utils::OpenGL::setScreenSizedViewport();
        Utils::OpenGL::setRenderState(true, true, true);
        glUseProgram(triangleDebugProgram);
        coreEngine->display();
    }
};