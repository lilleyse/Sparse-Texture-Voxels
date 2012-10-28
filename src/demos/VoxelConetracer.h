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
        std::string vertexShaderSource = SHADER_DIRECTORY + "fullscreenQuad.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "conetracerDemo.frag";
        fullScreenProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }

    void display()
    {
        Utils::OpenGL::setScreenSizedViewport();
        Utils::OpenGL::setRenderState(true, true, true);
        glUseProgram(fullScreenProgram);
        fullScreenQuad->display();
    }
};
