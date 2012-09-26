#pragma once

#include <glf.hpp>
#include "Utils.h"

struct TextureData
{
    std::vector<glm::u8vec4> colorData;
    std::vector<glm::vec3> normalData;
};

class VoxelTexture
{

public:

    GLuint colorTexture;
    GLuint normalTexture;
    GLuint texture3DSampler;

    uint voxelGridLength;
    uint numMipMapLevels;

    void begin(uint voxelGridLength, uint numMipMapLevels)
    {
        //Create a default empty vector for each texture data
        this->voxelGridLength = voxelGridLength;
        this->numMipMapLevels = numMipMapLevels;

        // Create a sampler to sample from any of the 3D textures
        glGenSamplers(1, &texture3DSampler);
        glBindSampler(COLOR_TEXTURE_3D_BINDING, texture3DSampler);
        glBindSampler(NORMAL_TEXTURE_3D_BINDING, texture3DSampler);

        float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glSamplerParameterfv(texture3DSampler, GL_TEXTURE_BORDER_COLOR, zeroes);
        glSamplerParameteri(texture3DSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(texture3DSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glSamplerParameteri(texture3DSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(texture3DSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(texture3DSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        // Create a dense 3D color texture
        glGenTextures(1, &colorTexture);
        glActiveTexture(GL_TEXTURE0 + COLOR_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, colorTexture);
        glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8, voxelGridLength, voxelGridLength, voxelGridLength);
        
        // Create a dense 3D normals texture
        glGenTextures(1, &normalTexture);
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, normalTexture);
        glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGB32F, voxelGridLength, voxelGridLength, voxelGridLength);

    }

    // Data is assumed to be in RGBA format
    void setData(TextureData& textureData)
    {
        glBindTexture(GL_TEXTURE_3D, colorTexture);
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, voxelGridLength, voxelGridLength, voxelGridLength, GL_RGBA, GL_UNSIGNED_BYTE, &textureData.colorData[0]);
   
        glBindTexture(GL_TEXTURE_3D, normalTexture);
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, voxelGridLength, voxelGridLength, voxelGridLength, GL_RGB, GL_FLOAT, &textureData.normalData[0]);
    }
};