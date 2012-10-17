#pragma once

#include "../Utils.h"

struct ShaderLibrary
{
    // Setup shaders
    GLuint mainPassShader;
    GLuint voxelDebugShader;

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

        /*GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "mainDeferred.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "mainDeferred.frag");

        voxelDebugShader = glCreateProgram();
        glAttachShader(voxelDebugShader, vertexShaderObject);
        glAttachShader(voxelDebugShader, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(voxelDebugShader);
        Utils::OpenGL::checkProgram(voxelDebugShader);*/
    }
};