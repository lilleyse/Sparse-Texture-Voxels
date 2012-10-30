#pragma once

#include "Utils.h"

struct TextureData
{
    std::vector<glm::u8vec4> colorData;
    std::vector<glm::i8vec4> normalData;
};

struct MipMapInfo
{
    uint offset;
    uint numVoxels;
    uint gridLength;
};

class VoxelTexture
{
public:

    GLuint colorTexture;
    GLuint normalTexture;
    
    // Samplers
    enum SamplerType {LINEAR, NEAREST, MAX_SAMPLER_TYPES};
    GLuint textureNearestSampler;
    GLuint textureLinearSampler;
    SamplerType currentSamplerType;

    uint voxelGridLength;
    uint numMipMapLevels;

    uint totalVoxels;
    std::vector<MipMapInfo> mipMapInfoArray;

    void begin(uint voxelGridLength, uint numMipMapLevels)
    {
        //Create a default empty vector for each texture data
        this->voxelGridLength = voxelGridLength;

        // Set num mipmaps based on the grid length
        if(numMipMapLevels == 0)
            numMipMapLevels = (uint)(glm::log2(float(voxelGridLength)) + 1.5);
        this->numMipMapLevels = numMipMapLevels;
        int baseLevel = 0;
        int maxLevel = numMipMapLevels-1;

        // Samplers
        float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};

        // Create a nearest sampler to sample from any of the 3D textures
        glGenSamplers(1, &textureNearestSampler);
        glBindSampler(NON_USED_TEXTURE, textureNearestSampler);
        glSamplerParameterfv(textureNearestSampler, GL_TEXTURE_BORDER_COLOR, zeroes);
        glSamplerParameterf(textureNearestSampler, GL_TEXTURE_MIN_LOD, (float)baseLevel);
        glSamplerParameterf(textureNearestSampler, GL_TEXTURE_MAX_LOD, (float)maxLevel);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(textureNearestSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        // Create a linear sampler to sample from any of the 3
        glGenSamplers(1, &textureLinearSampler);
        glBindSampler(NON_USED_TEXTURE, textureLinearSampler);
        glSamplerParameterfv(textureLinearSampler, GL_TEXTURE_BORDER_COLOR, zeroes);
        glSamplerParameterf(textureNearestSampler, GL_TEXTURE_MIN_LOD, (float)baseLevel);
        glSamplerParameterf(textureNearestSampler, GL_TEXTURE_MAX_LOD, (float)maxLevel);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(textureLinearSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        this->setSamplerType(LINEAR);

        // Create a dense 3D color texture
        glActiveTexture(GL_TEXTURE0 + COLOR_TEXTURE_3D_BINDING);
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_3D, colorTexture);
        glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8, voxelGridLength, voxelGridLength, voxelGridLength);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, baseLevel); 
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, maxLevel);

        // Create a dense 3D normals texture
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_3D_BINDING);
        glGenTextures(1, &normalTexture);
        glBindTexture(GL_TEXTURE_3D, normalTexture);
        glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8_SNORM, voxelGridLength, voxelGridLength, voxelGridLength);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, baseLevel); 
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, maxLevel);

        // Store empty data in the voxel texture
        uint voxelTextureSize = voxelGridLength * voxelGridLength * voxelGridLength;
        TextureData emptyData;
        emptyData.colorData.resize(voxelTextureSize, glm::u8vec4(0,0,0,0));
        emptyData.normalData.resize(voxelTextureSize, glm::i8vec4(0,0,0,0));
        setData(emptyData, voxelGridLength, 0);

        // Store mipmap data
        int numVoxels = 0;
        int mipMapSideLength = voxelGridLength;
        for(uint i = 0; i < numMipMapLevels; i++)
        {
            MipMapInfo mipMapInfo;
            mipMapInfo.offset = numVoxels;
            mipMapInfo.numVoxels = mipMapSideLength*mipMapSideLength*mipMapSideLength;
            mipMapInfo.gridLength = mipMapSideLength;

            mipMapInfoArray.push_back(mipMapInfo);
            mipMapSideLength /= 2;
            numVoxels += mipMapInfo.numVoxels;
        }
        totalVoxels = numVoxels;
    }

    // Data is assumed to be in RGBA format
    void setData(TextureData& textureData, uint sideLength, uint mipMapLevel)
    {
        glActiveTexture(GL_TEXTURE0 + COLOR_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, colorTexture);
        glTexSubImage3D(GL_TEXTURE_3D, mipMapLevel, 0, 0, 0, sideLength, sideLength, sideLength, GL_RGBA, GL_UNSIGNED_BYTE, &textureData.colorData[0]);
   
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, normalTexture);
        glTexSubImage3D(GL_TEXTURE_3D, mipMapLevel, 0, 0, 0, sideLength, sideLength, sideLength, GL_RGBA, GL_BYTE, &textureData.normalData[0]);
    }

    void setSamplerType(SamplerType samplerType)
    {
        this->currentSamplerType = samplerType;
        if (currentSamplerType == LINEAR)
        {
            glBindSampler(COLOR_TEXTURE_3D_BINDING, textureLinearSampler);
            glBindSampler(NORMAL_TEXTURE_3D_BINDING, textureLinearSampler);
        }
        else if (currentSamplerType == NEAREST)
        {
            glBindSampler(COLOR_TEXTURE_3D_BINDING, textureNearestSampler);
            glBindSampler(NORMAL_TEXTURE_3D_BINDING, textureNearestSampler);
        }
    }
    void changeSamplerType()
    {
        uint position = (uint)currentSamplerType + 1;
        if (position >= (int)MAX_SAMPLER_TYPES)
            position = 0;
        setSamplerType((SamplerType)position);
    }
};