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

        glUseProgram(cleanProgram);

        for(uint i = 0; i < voxelTexture->numCascades; i++)
        {
            for(uint j = 0; j < NUM_VOXEL_DIRECTIONS; j++)
            {
                glBindImageTexture(VOXEL_DIRECTIONS_IMAGE_ARRAY_BINDING[j], voxelTexture->colorTextures[i*NUM_VOXEL_DIRECTIONS + j], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
            }
            fullScreenQuad->displayInstanced(voxelGridLength);
        }  
    }
};