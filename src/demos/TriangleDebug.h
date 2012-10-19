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
        // Create program that reads the deferred data
        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "mainDeferred.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "triangleDebug.frag");

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
        glUseProgram(triangleDebugProgram);
        coreEngine->display();
    }
};