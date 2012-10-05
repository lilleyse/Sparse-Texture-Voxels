#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "../FullScreenQuad.h"

class VoxelRaycaster
{

private:
    GLuint fullScreenProgram;
    VoxelTexture* voxelTexture;
    FullScreenQuad* fullScreenQuad;

public:
    VoxelRaycaster(){}
    virtual ~VoxelRaycaster(){}

    void begin(VoxelTexture* voxelTexture, FullScreenQuad* fullScreenQuad)
    {
        this->voxelTexture = voxelTexture;
        this->fullScreenQuad = fullScreenQuad;

        // Create shader program
        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "fullscreen.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "raycast.frag");

        fullScreenProgram = glCreateProgram();
        glAttachShader(fullScreenProgram, vertexShaderObject);
        glAttachShader(fullScreenProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(fullScreenProgram);
        Utils::OpenGL::checkProgram(fullScreenProgram);
    }

    void display()
    {
        voxelTexture->enableNearestSampling();
        glUseProgram(fullScreenProgram);
        fullScreenQuad->display();
    }

    void setMipMapLevel(uint mipMapLevel)
    {
        glUseProgram(fullScreenProgram);
        GLuint mipMapLevelUniform = glGetUniformLocation(fullScreenProgram, "uMipLevel");
        glUniform1f(mipMapLevelUniform, (float)mipMapLevel);
    }
};
