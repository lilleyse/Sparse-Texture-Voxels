#pragma once

#include "Utils.h"
#include "ShaderConstants.h"
#include "VoxelTexture.h"
#include "FullScreenQuad.h"

class VoxelClean
{
private: 
    GLuint cleanProgram;
    VoxelTexture* voxelTexture;
    FullScreenQuad* fullScreenQuad;

public:
    void begin(VoxelTexture* voxelTexture, FullScreenQuad* fullScreenQuad)
    {
        this->voxelTexture = voxelTexture;
        this->fullScreenQuad = fullScreenQuad;

        std::string vertexShaderSource = SHADER_DIRECTORY + "fullscreenQuadInstanced.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "voxelClean.frag";
        cleanProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }

    void clean()
    {
        int voxelGridLength = voxelTexture->voxelGridLength;
        Utils::OpenGL::setViewport(voxelGridLength, voxelGridLength);
        Utils::OpenGL::setRenderState(false, false, false);
        glBindImageTexture(COLOR_IMAGE_3D_BINDING_BASE, voxelTexture->colorTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

        // Clean the base mip map
        glUseProgram(cleanProgram);
        fullScreenQuad->displayInstanced(voxelGridLength);
    }
};