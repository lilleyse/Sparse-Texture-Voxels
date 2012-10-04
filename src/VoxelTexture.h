#pragma once

#include "Utils.h"

struct TextureData
{
    std::vector<glm::u8vec4> colorData;
    std::vector<glm::vec4> normalData;
};

class VoxelTexture
{

public:

    GLuint colorTexture;
    GLuint normalTexture;
    GLuint textureNearestSampler;
    GLuint textureLinearSampler;

    uint voxelGridLength;
    uint numMipMapLevels;

    void begin(uint voxelGridLength)
    {
        //Create a default empty vector for each texture data
        this->voxelGridLength = voxelGridLength;
        this->numMipMapLevels = (uint)(glm::log2(float(voxelGridLength)) + 1.5);

        // Create a nearest sampler to sample from any of the 3D textures
        glGenSamplers(1, &textureNearestSampler);
        glBindSampler(0, textureNearestSampler);

        float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glSamplerParameterfv(textureNearestSampler, GL_TEXTURE_BORDER_COLOR, zeroes);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        // Create a linear sampler to sample from any of the 3
        glGenSamplers(1, &textureLinearSampler);
        glBindSampler(0, textureLinearSampler);

        glSamplerParameterfv(textureLinearSampler, GL_TEXTURE_BORDER_COLOR, zeroes);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        // Create a dense 3D color texture
        glActiveTexture(GL_TEXTURE0 + COLOR_TEXTURE_3D_BINDING);
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_3D, colorTexture);
        glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8, voxelGridLength, voxelGridLength, voxelGridLength);
        
        // Create a dense 3D normals texture
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_3D_BINDING);
        glGenTextures(1, &normalTexture);
        glBindTexture(GL_TEXTURE_3D, normalTexture);
        glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA32F, voxelGridLength, voxelGridLength, voxelGridLength);
        
        // Store empty data in the voxel texture
        uint voxelTextureSize = voxelGridLength * voxelGridLength * voxelGridLength;
        TextureData emptyData;
        emptyData.colorData.resize(voxelTextureSize, glm::u8vec4(0,0,0,0));
        emptyData.normalData.resize(voxelTextureSize);
        setData(emptyData, voxelGridLength, 0);
    }

    // Data is assumed to be in RGBA format
    void setData(TextureData& textureData, uint sideLength, uint mipMapLevel)
    {
        glActiveTexture(GL_TEXTURE0 + COLOR_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, colorTexture);
        glTexSubImage3D(GL_TEXTURE_3D, mipMapLevel, 0, 0, 0, sideLength, sideLength, sideLength, GL_RGBA, GL_UNSIGNED_BYTE, &textureData.colorData[0]);
   
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, normalTexture);
        glTexSubImage3D(GL_TEXTURE_3D, mipMapLevel, 0, 0, 0, sideLength, sideLength, sideLength, GL_RGBA, GL_FLOAT, &textureData.normalData[0]);
    }

    void enableLinearSampling()
    {
        glBindSampler(COLOR_TEXTURE_3D_BINDING, textureLinearSampler);
        glBindSampler(NORMAL_TEXTURE_3D_BINDING, textureLinearSampler);
    }

    void enableNearestSampling()
    {
        glBindSampler(COLOR_TEXTURE_3D_BINDING, textureNearestSampler);
        glBindSampler(NORMAL_TEXTURE_3D_BINDING, textureNearestSampler);
    }

    void display()
    {
        // Rebind to the binding points in case binding points were messed up (like in MipMapGenerator)
        glActiveTexture(GL_TEXTURE0 + COLOR_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, colorTexture);
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, normalTexture);
    }
};