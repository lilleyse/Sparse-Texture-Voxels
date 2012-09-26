#pragma once

#include <glf.hpp>
#include <map>
#include <utility>

#include "Utils.h"
#include "MipMapGenerator.h"
#include "VoxelTexture.h"

class VoxelTextureGenerator
{
private:

    VoxelTexture* voxelTexture;
    bool loadMulitpleTextures;

    std::map<std::string, uint> textureNamesToIndexes;
    std::vector<TextureData> textures;
    uint currentTexture;

    MipMapGenerator mipMapGenerator;

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

    void createTexture(std::string name)
    {
        if (textureNamesToIndexes.find(name) == textureNamesToIndexes.end() && (textures.size() == 0 || loadMulitpleTextures))
        {
            if (name == CUBE_PRESET)
            {
                uint voxelGridLength = voxelTexture->voxelGridLength;
                std::vector<glm::u8vec4> colorData = std::vector<glm::u8vec4>(voxelGridLength*voxelGridLength*voxelGridLength);
                std::vector<glm::vec3> normalData = std::vector<glm::vec3>(voxelGridLength*voxelGridLength*voxelGridLength);                
                
                uint textureIndex = 0;
                uint half = voxelGridLength / 2;
                for(uint i = 0; i < voxelGridLength; i++)
                for(uint j = 0; j < voxelGridLength; j++)
                for(uint k = 0; k < voxelGridLength; k++) 
                {
                    if (i<half && j<half && k<half)
                        colorData[textureIndex] = glm::u8vec4(255,0,0,127);
                    else if (i>=half && j<half && k<half)
                        colorData[textureIndex] = glm::u8vec4(0,255,0,127);
                    else if (i<half && j>=half && k<half)
                        colorData[textureIndex] = glm::u8vec4(0,0,255,127);
                    else if (i>=half && j>=half && k<half)
                        colorData[textureIndex] = glm::u8vec4(255,255,255,127);
                    else
                        colorData[textureIndex] = glm::u8vec4(127,127,127,127);
                    normalData[textureIndex] = glm::vec3(0,0,0);
                    textureIndex++;
                }
                textureNamesToIndexes.insert(std::pair<std::string, uint>(name, textures.size()));
                
                TextureData textureData;
                textureData.colorData = colorData;
                textureData.normalData = normalData;
                textures.push_back(textureData);

            }
            else if (name == SPHERE_PRESET)
            {
                uint voxelGridLength = voxelTexture->voxelGridLength;
                glm::vec3 center = glm::vec3(voxelGridLength/2);
                float radius = voxelGridLength/2.0f;

                std::vector<glm::u8vec4> colorData = std::vector<glm::u8vec4>(voxelGridLength*voxelGridLength*voxelGridLength);
                std::vector<glm::vec3> normalData = std::vector<glm::vec3>(voxelGridLength*voxelGridLength*voxelGridLength);
                uint textureIndex = 0;
                for(uint i = 0; i < voxelGridLength; i++)
                for(uint j = 0; j < voxelGridLength; j++)
                for(uint k = 0; k < voxelGridLength; k++) 
                {
                    float distanceFromCenter = glm::distance(center, glm::vec3(i,j,k));
				    if(distanceFromCenter < radius)
                    {
                        colorData[textureIndex] = glm::u8vec4(((float)i/voxelGridLength)*255.0f, ((float)j/voxelGridLength)*255.0f, ((float)k/voxelGridLength)*255.0f, 255);
                        normalData[textureIndex] = glm::normalize(glm::vec3(glm::vec3(i,j,k) - center));
                    }
                    else
                    {
                        colorData[textureIndex] = glm::u8vec4(0,0,0,0);
                        normalData[textureIndex] = glm::vec3(0,0,0);
                    }
                    textureIndex++;
                }
                textureNamesToIndexes.insert(std::pair<std::string, uint>(name, textures.size()));
                
                TextureData textureData;
                textureData.colorData = colorData;
                textureData.normalData = normalData;
                textures.push_back(textureData);
            }
            else 
            {
                //handle file loading
            }
        }
    }

public:
    std::string CUBE_PRESET;
    std::string SPHERE_PRESET;
    VoxelTextureGenerator()
    {
        CUBE_PRESET = "cube";
        SPHERE_PRESET = "sphere";
        currentTexture = -1;
    }

    void begin(uint voxelGridLength, uint numMipMapLevels, bool loadMultipleTextures) 
    {   
        this->loadMulitpleTextures = loadMultipleTextures;

        voxelTexture = new VoxelTexture();
        voxelTexture->begin(voxelGridLength, numMipMapLevels);

        if (loadMultipleTextures) 
        {
            createTexture(CUBE_PRESET);
            createTexture(SPHERE_PRESET);        
        }
    }

    bool setTexture(std::string name)
    { 
        // Adds the texture's data to the gpu.
        createTexture(name); // If the texture data doesn't exist, this will create it
        std::map<std::string, uint>::iterator iter = textureNamesToIndexes.find(name);
        if (iter != textureNamesToIndexes.end())
        {
            return setTexture(iter->second);
        }
        return false;
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