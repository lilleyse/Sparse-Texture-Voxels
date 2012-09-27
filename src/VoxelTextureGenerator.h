#pragma once

#include <glf.hpp>
#include <map>
#include <utility>
#include <iostream>
#include <fstream>
#include <limits>

#include "Utils.h"
#include "VoxelTexture.h"
#include "MipMapGenerator.h"

class VoxelTextureGenerator
{
private:

    MipMapGenerator mipMapGenerator;
    VoxelTexture* voxelTexture;
    std::map<std::string, uint> textureNamesToIndexes;
    std::vector<TextureData> textures;
    bool loadMultipleTextures;
    uint currentTexture;

public:

    std::string CUBE_PRESET;
    std::string SPHERE_PRESET;
    VoxelTextureGenerator()
    {
        CUBE_PRESET = "cube";
        SPHERE_PRESET = "sphere";
        currentTexture = UINT_MAX;
    }

    void begin(uint voxelGridLength, bool loadMultipleTextures) 
    {   
        this->loadMultipleTextures = loadMultipleTextures;
        voxelTexture = new VoxelTexture();
        voxelTexture->begin(voxelGridLength);
    }

    void createAllPresets()
    {
        createTexture(CUBE_PRESET);
        createTexture(SPHERE_PRESET);  
    }
    void createTexture(std::string name)
    {
        uint voxelGridLength = voxelTexture->voxelGridLength;
        uint voxelTextureSize = voxelGridLength*voxelGridLength*voxelGridLength;

        TextureData textureData;
        textureData.colorData.resize(voxelTextureSize);
        textureData.normalData.resize(voxelTextureSize);
        

        if (textureNamesToIndexes.find(name) == textureNamesToIndexes.end() && (textures.size() == 0 || this->loadMultipleTextures))
        {
            if (name == CUBE_PRESET)
            {
                uint textureIndex = 0;
                uint half = voxelGridLength / 2;
                for(uint i = 0; i < voxelGridLength; i++)
                for(uint j = 0; j < voxelGridLength; j++)
                for(uint k = 0; k < voxelGridLength; k++) 
                {
                    if (i<half && j<half && k<half)
                        textureData.colorData[textureIndex] = glm::u8vec4(255,0,0,127);
                    else if (i>=half && j<half && k<half)
                        textureData.colorData[textureIndex] = glm::u8vec4(0,255,0,127);
                    else if (i<half && j>=half && k<half)
                        textureData.colorData[textureIndex] = glm::u8vec4(0,0,255,127);
                    else if (i>=half && j>=half && k<half)
                        textureData.colorData[textureIndex] = glm::u8vec4(255,255,255,127);
                    else
                        textureData.colorData[textureIndex] = glm::u8vec4(127,127,127,127);
                    
                    textureData.normalData[textureIndex] = glm::vec3(0,0,0);
                    textureIndex++;
                }
            }
            else if (name == SPHERE_PRESET)
            {
                uint textureIndex = 0;
                glm::vec3 center = glm::vec3(voxelGridLength/2);
                float radius = voxelGridLength/2.0f;
                for(uint i = 0; i < voxelGridLength; i++)
                for(uint j = 0; j < voxelGridLength; j++)
                for(uint k = 0; k < voxelGridLength; k++) 
                {
                    float distanceFromCenter = glm::distance(center, glm::vec3(i,j,k));
				    if(distanceFromCenter < radius)
                    {
                        textureData.colorData[textureIndex] = glm::u8vec4(((float)i/voxelGridLength)*255.0f, ((float)j/voxelGridLength)*255.0f, ((float)k/voxelGridLength)*255.0f, 255);
                        textureData.normalData[textureIndex] = glm::normalize(glm::vec3(glm::vec3(i,j,k) - center));
                    }
                    else
                    {
                        textureData.colorData[textureIndex] = glm::u8vec4(0,0,0,0);
                        textureData.normalData[textureIndex] = glm::vec3(0,0,0);
                    }
                    textureIndex++;
                }
            }
            else 
            {
                // try to load a raw voxel texture
                std::ifstream file(name, std::ios::in|std::ios::binary);
                if (!file) return;
                uchar* buffer = new uchar[voxelTextureSize];
                file.read((char*)buffer, voxelTextureSize);

                // store one channel buffer into four channel texture data
                for (uint i = 0; i < voxelTextureSize; i++)
                    textureData.colorData[i] = glm::u8vec4(255, 255, 255, buffer[i]);

                delete[] buffer;
                file.close();
            }

            textureNamesToIndexes.insert(std::pair<std::string, uint>(name, textures.size()));
            textures.push_back(textureData);
        }
    }

    
    bool setTexture(std::string name)
    { 
        std::map<std::string, uint>::iterator iter = textureNamesToIndexes.find(name);
        if (iter != textureNamesToIndexes.end())
            return setTexture(iter->second);
        return false;
    }

    bool setTexture(int textureIndex)
    {     
        if (textureIndex < 0) textureIndex = textures.size() - 1;
        if (textureIndex >= (int)textures.size()) textureIndex = 0;
        if (textureIndex == currentTexture) return false;
        currentTexture = textureIndex;
            
        // Fill entire texture (first mipmap level) then create mipmaps
        voxelTexture->setData(textures.at(currentTexture), voxelTexture->voxelGridLength, 0);
        mipMapGenerator.generateMipMapCPU(voxelTexture);
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
    VoxelTexture* getVoxelTexture()
    {
        return voxelTexture;
    }
};