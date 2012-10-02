#pragma once

#include <glf.hpp>

#include "../ShaderConstants.h"
#include "../Utils.h"
#include "../FullScreenQuad.h"
#include "../VoxelTexture.h"

class VoxelConetracer
{

private:
    GLuint fullScreenProgram;

public:
    VoxelConetracer(){}
    virtual ~VoxelConetracer(){}

    void begin(VoxelTexture* voxelTexture)
    {
        // Create shader program
        GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "fullscreen.vert");
        GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "conetrace.frag");

        fullScreenProgram = glCreateProgram();
        glAttachShader(fullScreenProgram, vertexShaderObject);
        glAttachShader(fullScreenProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(fullScreenProgram);
        glf::checkProgram(fullScreenProgram);
        
        glUseProgram(fullScreenProgram);
        GLuint mipMapLevelUniform = glGetUniformLocation(fullScreenProgram, "uTextureRes");
        glUniform1f(mipMapLevelUniform, (float)voxelTexture->voxelGridLength);
    }

    void display(FullScreenQuad& fullScreenQuad)
    {
        glUseProgram(fullScreenProgram);
        fullScreenQuad.display();
    }
};
