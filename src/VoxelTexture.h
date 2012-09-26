#pragma once

#include <glf.hpp>
#include "Utils.h"

class VoxelTexture
{

public:

    GLuint textureGL;
    uint voxelGridLength;
    uint numMipMapLevels;

    void begin(uint voxelGridLength, uint numMipMapLevels)
    {
        //Create a default empty vector for each texture data
        this->voxelGridLength = voxelGridLength;
        this->numMipMapLevels = numMipMapLevels;

        // Create a dense 3D texture
        glGenTextures(1, &textureGL);
        glActiveTexture(GL_TEXTURE0 + VOXEL_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, textureGL);
 
        glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8, voxelGridLength, voxelGridLength, voxelGridLength);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, zeroes);
    }

    // Data is assumed to be in RGBA format
    void setData(void* data)
    {
        glBindTexture(GL_TEXTURE_3D, textureGL);
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, voxelGridLength, voxelGridLength, voxelGridLength, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
};