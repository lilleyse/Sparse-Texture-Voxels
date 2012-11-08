#pragma once

#include "Utils.h"
#include "ShaderConstants.h"
#include "VoxelTexture.h"
#include "FullScreenQuad.h"

class MipMapGenerator
{
private:

    GLuint mipmapProgram;
    VoxelTexture* voxelTexture;
    FullScreenQuad* fullScreenQuad;
    PerFrameUBO* perFrame;
    GLuint perFrameUBO;

public:

    void begin(VoxelTexture* voxelTexture, FullScreenQuad* fullScreenQuad, PerFrameUBO* perFrame, GLuint perFrameUBO)
    {
        this->voxelTexture = voxelTexture;
        this->fullScreenQuad = fullScreenQuad;
        this->perFrame = perFrame;
        this->perFrameUBO = perFrameUBO;

        // Create mipmap shader program
        std::string vertexShaderSource = SHADER_DIRECTORY + "fullscreenQuadInstanced.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "mipmap.frag";
        mipmapProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }

    void generateMipMapGPU()
    {
        // Disable culling, depth test, rendering
        Utils::OpenGL::setRenderState(false, false, false);

        // Mip-map
        glUseProgram(mipmapProgram);
        glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
        int voxelGridLength = voxelTexture->voxelGridLength;
        for(uint i = 1; i < voxelTexture->numMipMapLevels; i++)
        {   
            perFrame->uCurrentMipLevel = i;
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);

            // Bind the six texture directions for writing
            for(uint j = 0; j < voxelTexture->NUM_DIRECTIONS; j++)
                glBindImageTexture(COLOR_IMAGE_POSX_3D_BINDING + j, voxelTexture->colorTextures[j], i, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

            // Call the program for each mip map level.
            int voxelGridLength = voxelTexture->mipMapInfoArray[i].gridLength;
            Utils::OpenGL::setViewport(voxelGridLength, voxelGridLength);
            fullScreenQuad->displayInstanced(voxelGridLength);
        }
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
};