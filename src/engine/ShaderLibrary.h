#pragma once
#include <glf.hpp>

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

        GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "mainDeferred.vert");
        GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "mainDeferred.frag");

        debugDrawShader = glCreateProgram();
        glAttachShader(debugDrawShader, vertexShaderObject);
        glAttachShader(debugDrawShader, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(debugDrawShader);
        glf::checkProgram(debugDrawShader);
    }
};