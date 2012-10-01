#pragma once

#include <glf.hpp>

#include "../ShaderConstants.h"
#include "../Utils.h"
#include "../FullScreenQuad.h"

class VoxelRaycaster
{

private:
    GLuint fullScreenProgram;

public:
    VoxelRaycaster(){}
    virtual ~VoxelRaycaster(){}

    void begin()
    {
        // Create shader program
        GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "fullscreen.vert");
        GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "raycast.frag");

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

    void setMipMapLevel(uint mipMapLevel)
    {
        glUseProgram(fullScreenProgram);
        GLuint mipMapLevelUniform = glGetUniformLocation(fullScreenProgram, "uMipLevel");
        glUniform1f(mipMapLevelUniform, (float)mipMapLevel);
    }
};
