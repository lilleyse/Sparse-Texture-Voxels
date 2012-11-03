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

        GLuint vertexShaderObject = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "fullscreenQuadInstanced.vert");
        GLuint fragmentShaderObject = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "voxelClean.frag");

        cleanProgram = glCreateProgram();
        glAttachShader(cleanProgram, vertexShaderObject);
        glAttachShader(cleanProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(cleanProgram);
        Utils::OpenGL::checkProgram(cleanProgram);
    }

    void clean()
    {
        // Change viewport to match the size of the second mip map level
        int oldViewport[4];
        glGetIntegerv(GL_VIEWPORT, oldViewport);

        // Disable culling, depth test, rendering
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        // Bind voxelTexture's color texture for writing
        glBindImageTexture(COLOR_IMAGE_3D_BINDING_BASE, voxelTexture->colorTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

        // First clean the base mip map
        int voxelGridLength = voxelTexture->voxelGridLength;
        glViewport(0, 0, voxelGridLength, voxelGridLength);
        glUseProgram(cleanProgram);
        fullScreenQuad->displayInstanced(voxelGridLength);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);//GL_TEXTURE_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Turn back on
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
};