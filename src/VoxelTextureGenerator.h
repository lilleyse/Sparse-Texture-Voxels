#pragma once

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
    uint currentTexture;

public:

    static std::string CUBE;
    static std::string SPHERE;
    static std::string CORNELL_BOX;

    void begin(VoxelTexture* voxelTexture) 
    {
        currentTexture = UINT_MAX;
        this->voxelTexture = voxelTexture;
    }
    void createTexture(std::string name)
    {
        uint voxelGridLength = voxelTexture->voxelGridLength;
        uint voxelTextureSize = voxelGridLength*voxelGridLength*voxelGridLength;

        TextureData textureData;
        textureData.colorData.resize(voxelTextureSize);
        textureData.normalData.resize(voxelTextureSize);
        
        if (textureNamesToIndexes.find(name) == textureNamesToIndexes.end())
        {
            if (name == CUBE)
            {
                uint textureIndex = 0;
                uint half = voxelGridLength / 2;                
                for(uint k = 0; k < voxelGridLength; k++)
                for(uint j = 0; j < voxelGridLength; j++)
                for(uint i = 0; i < voxelGridLength; i++) 
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
                    
                    textureData.normalData[textureIndex] = glm::vec4(0,0,0,0);
                    textureIndex++;
                }
            }
            else if (name == SPHERE)
            {
                uint textureIndex = 0;
                glm::vec3 center = glm::vec3(voxelGridLength/2-0.5);
                float radius = voxelGridLength/2.0f;
                for(uint k = 0; k < voxelGridLength; k++)
                for(uint j = 0; j < voxelGridLength; j++)                
                for(uint i = 0; i < voxelGridLength; i++)
                {
                    float distanceFromCenter = glm::distance(center, glm::vec3(i,j,k));
				    if(distanceFromCenter < radius)
                    {
                        textureData.colorData[textureIndex] = glm::u8vec4(((float)i/voxelGridLength)*255.0f, ((float)j/voxelGridLength)*255.0f, ((float)k/voxelGridLength)*255.0f, 255);
                        textureData.normalData[textureIndex] = glm::vec4(glm::normalize(glm::vec3(glm::vec3(i,j,k) - center)), 0);
                    }
                    else
                    {
                        // "cleaner" texture, no black edges during linear interp, not sure if we should do this
                        textureData.colorData[textureIndex] = glm::u8vec4(((float)i/voxelGridLength)*255.0f, ((float)j/voxelGridLength)*255.0f, ((float)k/voxelGridLength)*255.0f, 0);

                        //textureData.colorData[textureIndex] = glm::u8vec4(0,0,0,0);
                        textureData.normalData[textureIndex] = glm::vec4(0,0,0,0);
                    }
                    textureIndex++;
                }
            }
            else if (name == CORNELL_BOX)
            {
                glm::vec3 sphereCenter = glm::vec3(voxelGridLength/4, voxelGridLength/4, voxelGridLength/4);
                float sphereRadius = voxelGridLength/4.0f;

                uint textureIndex = 0;
                for(uint k = 0; k < voxelGridLength; k++)                
                for(uint j = 0; j < voxelGridLength; j++)
                for(uint i = 0; i < voxelGridLength; i++)
                {
                    float sphereDist = glm::distance(sphereCenter, glm::vec3(i,j,k));

                    if (j==0 || 
                        j==voxelGridLength-1 ||
                        k==0 ) {
                        textureData.colorData[textureIndex] = glm::u8vec4(255.0f,255.0f,255.0f,255.0f);
                    }
				    else if (i==0) {
                        textureData.colorData[textureIndex] = glm::u8vec4(255.0f,0,0,255.0f);
                    }
                    else if (i==voxelGridLength-1) {
                        textureData.colorData[textureIndex] = glm::u8vec4(0,255.0f,0,255.0f);
                    }
                    else if (sphereDist<sphereRadius) {
                        textureData.colorData[textureIndex] = glm::u8vec4(255.0f,255.0f,0.0f,255.0f);
                    }
                    else {
                        textureData.colorData[textureIndex] = glm::u8vec4(0,0,0,0);
                    }

                    // not doing anything
                    textureData.normalData[textureIndex] = glm::vec4(0,0,0,0);
                    textureIndex++;
                }
            }
            else 
            {
                std::string extension = name.substr(name.find_last_of(".") + 1);
                if (extension == "raw")
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
                else
                {
                    return;
                }
            }

            textureNamesToIndexes.insert(std::pair<std::string, uint>(name, textures.size()));
            textures.push_back(textureData);
        }
    }

    void createTextureFromVoxelTexture(std::string name)
    {
        uint voxelGridLength = voxelTexture->voxelGridLength;
        uint voxelTextureSize = voxelGridLength*voxelGridLength*voxelGridLength;

        TextureData textureData;
        textureData.colorData.resize(voxelTextureSize);
        textureData.normalData.resize(voxelTextureSize);

        glActiveTexture(GL_TEXTURE0 + COLOR_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, voxelTexture->colorTexture);
        glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &textureData.colorData[0]);

        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_3D_BINDING);
        glBindTexture(GL_TEXTURE_3D, voxelTexture->normalTexture);
        glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, &textureData.normalData[0]);

        textureNamesToIndexes.insert(std::pair<std::string, uint>(name, textures.size()));
        textures.push_back(textureData);
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
};

std::string VoxelTextureGenerator::CUBE = "cube";
std::string VoxelTextureGenerator::SPHERE = "sphere";
std::string VoxelTextureGenerator::CORNELL_BOX = "cornell box";
