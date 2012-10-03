#pragma once

#include "../Utils.h"

struct ShaderLibrary
{
    // Setup shaders
    GLuint mainPassShader;
    GLuint debugDrawShader;

    void begin()
    {
        /*
        // Create main pass shader
        GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "mainPass.vert");
        GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "mainPass.frag");

        mainPassShader = glCreateProgram();
        glAttachShader(mainPassShader, vertexShaderObject);
        glAttachShader(mainPassShader, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(mainPassShader);
        glf::checkProgram(mainPassShader);
        */

        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "mainDeferred.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "mainDeferred.frag");

        debugDrawShader = glCreateProgram();
        glAttachShader(debugDrawShader, vertexShaderObject);
        glAttachShader(debugDrawShader, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(debugDrawShader);
        Utils::OpenGL::checkProgram(debugDrawShader);
    }
};