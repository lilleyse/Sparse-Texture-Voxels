#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "../FullScreenQuad.h"

class VoxelRaycaster
{

private:
    GLuint fullScreenProgram;
    FullScreenQuad* fullScreenQuad;

public:
    VoxelRaycaster(){}
    virtual ~VoxelRaycaster(){}

    void begin(FullScreenQuad* fullScreenQuad)
    {
        this->fullScreenQuad = fullScreenQuad;

        // Create shader program
        std::string vertexShaderSource = SHADER_DIRECTORY + "fullscreenQuad.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "raycasterDemo.frag";
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
