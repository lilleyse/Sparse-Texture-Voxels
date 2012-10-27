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

        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "triangleProcessor.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "triangleDebugDemo.frag");

        triangleDebugProgram = glCreateProgram();
        glAttachShader(triangleDebugProgram, vertexShaderObject);
        glAttachShader(triangleDebugProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(triangleDebugProgram);
        Utils::OpenGL::checkProgram(triangleDebugProgram);
    }

    void display()
    {
        Utils::OpenGL::setScreenSizedViewport();
        Utils::OpenGL::setRenderState(true, true, true);
        glUseProgram(triangleDebugProgram);
        coreEngine->display();
    }
};