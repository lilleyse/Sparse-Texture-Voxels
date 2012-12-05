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

    void generateMipMap()
    {
        // Disable culling, depth test, rendering
        Utils::OpenGL::setRenderState(false, false, false);

        // Mip-map
        glUseProgram(mipmapProgram);
        glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);

        for(uint i = 0; i < voxelTexture->numCascades; i++)
        {
            perFrame->uCurrentCascade = i;
            for(uint j = 1; j < voxelTexture->numMipMapLevels; j++)
            {   
                perFrame->uCurrentMipLevel = j;
                glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
                // Bind the six texture directions for writing
                for(uint k = 0; k < NUM_VOXEL_DIRECTIONS; k++)
                {
                    glBindImageTexture(VOXEL_DIRECTIONS_IMAGE_ARRAY_BINDING[k], voxelTexture->colorTextures[i*NUM_VOXEL_DIRECTIONS + k], j, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
                    //rebind the first six textures due to shader glitch
                    glActiveTexture(GL_TEXTURE0 + VOXEL_DIRECTIONS_ARRAY_BINDING[k]);
                    glBindTexture(GL_TEXTURE_3D, this->voxelTexture->colorTextures[i*NUM_VOXEL_DIRECTIONS + k]);
                }

                // Call the program for each mip map level.
                int voxelGridLength = voxelTexture->mipMapInfoArray[j].gridLength;
                Utils::OpenGL::setViewport(voxelGridLength, voxelGridLength);
                fullScreenQuad->displayInstanced(voxelGridLength);
            }
        }

        //bind the correct textures back
        for(uint i = 0; i < NUM_VOXEL_DIRECTIONS; i++)
        {
            glActiveTexture(GL_TEXTURE0 + VOXEL_DIRECTIONS_ARRAY_BINDING[i]);
            glBindTexture(GL_TEXTURE_3D, this->voxelTexture->colorTextures[i]);
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
};