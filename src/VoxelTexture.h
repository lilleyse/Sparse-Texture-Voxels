#pragma once

#include "Utils.h"

struct MipMapInfo
{
    uint offset;
    uint numVoxels;
    uint gridLength;
};

class VoxelTexture
{
public:
    // Samplers
    enum SamplerType {LINEAR, NEAREST, MAX_SAMPLER_TYPES};
    GLuint textureNearestSampler;
    GLuint textureLinearSampler;
    SamplerType currentSamplerType;

    std::vector<GLuint> colorTextures;
    std::vector<MipMapInfo> mipMapInfoArray;

    uint voxelGridLength;
    uint numMipMapLevels;
    uint numCascades;
    uint numVoxelTextures;
    uint totalVoxels;
    
    void begin(uint voxelGridLength, uint numMipMapLevels, uint numCascades)
    {
        this->voxelGridLength = voxelGridLength;
        this->numCascades = numCascades;
        this->numVoxelTextures = this->numCascades * NUM_VOXEL_DIRECTIONS;

        // Set num mipmaps based on the grid length
        if(numMipMapLevels == 0)
            numMipMapLevels = (uint)(glm::log2(float(voxelGridLength)) + 1.5);
        this->numMipMapLevels = numMipMapLevels;
        int baseLevel = 0;
        int maxLevel = numMipMapLevels-1;

        // Samplers
        float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};

        // Nearest
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

        // Linear
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
        colorTextures.resize(this->numVoxelTextures);
        for (uint i = 0; i < this->numVoxelTextures; i++)
        {
            glActiveTexture(GL_TEXTURE0 + VOXEL_DIRECTIONS_ARRAY_BINDING[i]);
            glGenTextures(1, &colorTextures[i]);
            glBindTexture(GL_TEXTURE_3D, colorTextures[i]);
            glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8, voxelGridLength, voxelGridLength, voxelGridLength);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, baseLevel); 
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, maxLevel);
        }
        
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

        this->totalVoxels = numVoxels;
    }

    void setSamplerType(SamplerType samplerType)
    {
        this->currentSamplerType = samplerType;
        for (uint i = 0; i < this->numVoxelTextures; i++)
        {
            if (currentSamplerType == LINEAR)
                glBindSampler(VOXEL_DIRECTIONS_ARRAY_BINDING[i], textureLinearSampler);
            else if (currentSamplerType == NEAREST)
                glBindSampler(VOXEL_DIRECTIONS_ARRAY_BINDING[i], textureNearestSampler);
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