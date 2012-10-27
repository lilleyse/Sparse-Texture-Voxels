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
        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "passthrough.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "passthrough.frag");
        passthroughProgram = glCreateProgram();
        glAttachShader(passthroughProgram, vertexShaderObject);
        glAttachShader(passthroughProgram , fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);
        glLinkProgram(passthroughProgram);
        Utils::OpenGL::checkProgram(passthroughProgram);
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