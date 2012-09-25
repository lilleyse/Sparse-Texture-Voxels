#pragma once

#include <glf.hpp>
#include <map>
#include <utility>

#include "Utils.h"
#include "MipMapGenerator.h"

class VoxelTextureGenerator
{
private:
    GLuint voxelTexture;
    unsigned int voxelGridLength;
    unsigned int numMipMapLevels;
    bool loadMulitpleTextures;

    std::map<std::string, unsigned int> textureNamesToIndexes;
    std::vector<std::vector<glm::u8vec4>> textures;
    unsigned int currentTexture;

    MipMapGenerator mipMapGenerator;

public:
    std::string CUBE_PRESET;
    std::string SPHERE_PRESET;
    VoxelTextureGenerator()
    {
        CUBE_PRESET = "cube";
        SPHERE_PRESET = "sphere";
        currentTexture = -1;
    }

    void begin(unsigned int voxelGridLength, unsigned int numMipMapLevels, bool loadMultipleTextures) 
    {   
        //Create a default empty vector for each texture data
        this->voxelGridLength = voxelGridLength;
        this->numMipMapLevels = numMipMapLevels;
        this->loadMulitpleTextures = loadMultipleTextures;

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

        if (loadMultipleTextures) 
        {
            createTexture(CUBE_PRESET);
            createTexture(SPHERE_PRESET);        
        }
    }

    void createTexture(std::string name)
    {
        if (textureNamesToIndexes.find(name) == textureNamesToIndexes.end() && (textures.size() == 0 || loadMulitpleTextures))
        {
            if (name == CUBE_PRESET)
            {
                std::vector<glm::u8vec4> textureData = std::vector<glm::u8vec4>(voxelGridLength*voxelGridLength*voxelGridLength);
                unsigned int textureIndex = 0;
                unsigned int half = voxelGridLength / 2;
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
                textureNamesToIndexes.insert(std::pair<std::string, unsigned int>(name, textures.size()));
                textures.push_back(textureData);
            }
            else if (name == SPHERE_PRESET)
            {
                std::vector<glm::u8vec4> textureData = std::vector<glm::u8vec4>(voxelGridLength*voxelGridLength*voxelGridLength);
                unsigned int textureIndex = 0;
                for(unsigned int i = 0; i < voxelGridLength; i++)
                for(unsigned int j = 0; j < voxelGridLength; j++)
                for(unsigned int k = 0; k < voxelGridLength; k++) 
                {
                    textureData[textureIndex] = glm::u8vec4(i,j,k,255);
                    textureIndex++;
                }
                textureNamesToIndexes.insert(std::pair<std::string, unsigned int>(name, textures.size()));
                textures.push_back(textureData);
            }
            else 
            {
                //handle file loading
            }
        }
    }

    bool setTexture(std::string name)
    { 
        // Adds the texture's data to the gpu.
        createTexture(name); // If the texture data doesn't exist, this will create it
        std::map<std::string, unsigned int>::iterator iter = textureNamesToIndexes.find(name);
        if (iter != textureNamesToIndexes.end())
        {
            return setTexture(iter->second);
        }
        return false;
    }
    bool setTexture(int textureIndex)
    {     
        if (textureIndex < 0) textureIndex = textures.size() - 1;
        if (textureIndex >= textures.size()) textureIndex = 0;
        if (textureIndex == currentTexture) return false;
        currentTexture = textureIndex;
            
        // Fill entire texture (first mipmap level) then fill mipmaps
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, voxelGridLength, voxelGridLength, voxelGridLength, GL_RGBA, GL_UNSIGNED_BYTE, &(textures.at(currentTexture)[0]));
        mipMapGenerator.generateMipMapCPU(voxelTexture, voxelGridLength, numMipMapLevels);
        return true;
    }
    bool setNextTexture()
    {
        return setTexture((int)currentTexture + 1);
    }
    bool setPreviousTexture()
    {
        return setTexture((int)currentTexture - 1);
    }
    GLuint getVoxelTexture()
    {
        return voxelTexture;
    }
};