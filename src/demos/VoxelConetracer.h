#pragma once

#include <glf.hpp>

#include "ShaderConstants.h"
#include "Utils.h"
#include "../FullScreenQuad.h"

class VoxelConetracer
{

private:
    GLuint fullScreenProgram;

public:
    VoxelConetracer(){}
    virtual ~VoxelConetracer(){}

    void begin()
    {
        // Create shader program
        GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, "src/shaders/fullscreen.vert");
        GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, "src/shaders/conetrace.frag");

        fullScreenProgram = glCreateProgram();
        glAttachShader(fullScreenProgram, vertexShaderObject);
        glAttachShader(fullScreenProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(fullScreenProgram);
        glf::checkProgram(fullScreenProgram);
    }

    void display(FullScreenQuad& fullScreenQuad)
    {
        glUseProgram(fullScreenProgram);
        fullScreenQuad.display();
    }

    void setTextureResolution(uint res)
    {
        glUseProgram(fullScreenProgram);
        GLuint mipMapLevelUniform = glGetUniformLocation(fullScreenProgram, "uTextureRes");
        glUniform1f(mipMapLevelUniform, (float)res);
    }
};
