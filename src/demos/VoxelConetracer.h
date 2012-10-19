#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "../FullScreenQuad.h"
#include "../VoxelTexture.h"

class VoxelConetracer
{

private:
    GLuint fullScreenProgram;
    VoxelTexture* voxelTexture;
    FullScreenQuad* fullScreenQuad;

public:
    VoxelConetracer(){}
    virtual ~VoxelConetracer(){}

    void begin(VoxelTexture* voxelTexture, FullScreenQuad* fullScreenQuad)
    {
        this->voxelTexture = voxelTexture;
        this->fullScreenQuad = fullScreenQuad;

        // Create shader program
        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "fullscreen.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "conetrace.frag");

        fullScreenProgram = glCreateProgram();
        glAttachShader(fullScreenProgram, vertexShaderObject);
        glAttachShader(fullScreenProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(fullScreenProgram);
        Utils::OpenGL::checkProgram(fullScreenProgram);
        
        glUseProgram(fullScreenProgram);
        GLuint mipMapLevelUniform = glGetUniformLocation(fullScreenProgram, "uTextureRes");
        glUniform1f(mipMapLevelUniform, (float)voxelTexture->voxelGridLength);
    }

    void display()
    {
        glUseProgram(fullScreenProgram);
        fullScreenQuad->display();
    }
};
