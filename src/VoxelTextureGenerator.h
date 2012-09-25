#pragma once

#include <glf.hpp>

#include "Utils.h"
#include "MipMapGenerator.h"

class VoxelTextureGenerator
{
public:
    enum TextureType {CUBE, SPHERE, NUM_TEXTURE_TYPES};
    void begin(unsigned int voxelGridLength) 
    {   
        //Create a default empty vector for each texture data
        this->allVoxelTextureData = std::vector<std::vector<glm::u8vec4>>(NUM_TEXTURE_TYPES);
        this->voxelGridLength = voxelGridLength;
        this->numMipMapLevels = Utils::getNumMipMapLevels(voxelGridLength);

        // Create a dense 3D texture
        glGenTextures(1, &voxelTexture);
        glActiveTexture(GL_TEXTURE0 + VOXEL_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, voxelTexture);
        glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8, voxelGridLength, voxelGridLength, voxelGridLength);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, zeroes);
    }

    void createTexture(TextureType textureType)
    {
        std::vector<glm::u8vec4> textureData(voxelGridLength*voxelGridLength*voxelGridLength);
        
        if (textureType == CUBE)
        {
            unsigned int half = voxelGridLength / 2;
            unsigned int textureIndex = 0;
            for(unsigned int i = 0; i < voxelGridLength; i++)
            for(unsigned int j = 0; j < voxelGridLength; j++)
            for(unsigned int k = 0; k < voxelGridLength; k++) 
            {
                if (i<half && j<half && k<half)
                    textureData[textureIndex] = glm::u8vec4(255,0,0,127);
                else if (i>=half && j<half && k<half)
                    textureData[textureIndex] = glm::u8vec4(0,255,0,127);
                else if (i<half && j>=half && k<half)
                    textureData[textureIndex] = glm::u8vec4(0,0,255,127);
                else if (i>=half && j>=half && k<half)
                    textureData[textureIndex] = glm::u8vec4(255,255,255,127);
                else
                    textureData[textureIndex] = glm::u8vec4(127,127,127,127);

                textureIndex++;
            }
        }
        else if (textureType == SPHERE)
        {
            unsigned int textureIndex = 0;
            for(unsigned int i = 0; i < voxelGridLength; i++)
            for(unsigned int j = 0; j < voxelGridLength; j++)
            for(unsigned int k = 0; k < voxelGridLength; k++) 
            {
                textureData[textureIndex] = glm::vec4(255, 0, 0, 255);
                textureIndex++;
            }
        }

        // Fill entire texture (first mipmap level) then fill mipmaps
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, voxelGridLength, voxelGridLength, voxelGridLength, GL_RGBA, GL_UNSIGNED_BYTE, &textureData[0]);
        mipMapGenerator.generateMipMapCPU(voxelTexture, voxelGridLength, numMipMapLevels);
    }

    void selectTextureType(TextureType textureType)
    {
        this->currentTextureType = (TextureType)(std::min(std::max(0, (int)textureType), NUM_TEXTURE_TYPES - 1));
        std::vector<glm::u8vec4> currentTextureData = this->allVoxelTextureData.at(this->currentTextureType);
        if (currentTextureData.size() == 0) createTexture(this->currentTextureType);
    }

    TextureType getCurrentTextureType()
    {
        return this->currentTextureType;
    }
    GLuint getVoxelTexture()
    {
        return this->voxelTexture;
    }

private:
    GLuint voxelTexture;
    unsigned int voxelGridLength;
    unsigned int numMipMapLevels;

    TextureType currentTextureType;
    MipMapGenerator mipMapGenerator;
    std::vector<std::vector<glm::u8vec4>> allVoxelTextureData;
};