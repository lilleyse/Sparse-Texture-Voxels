#pragma once

#include "Utils.h"
#include "ShaderConstants.h"
#include "VoxelTexture.h"

class MipMapGenerator
{
private:

    uint indexConverter(uint sideLength, glm::uvec3 index3d)
    {
        return index3d.x + index3d.y*sideLength + index3d.z*sideLength*sideLength;
    }

public:

    // This code is not super efficient since it is a short term solution that will be replaced by GPU-based mipmap generation
    void generateMipMapCPU(VoxelTexture* voxelTexture)
    {
        int mipMapSideLength = voxelTexture->voxelGridLength;
        for(uint i = 1; i < voxelTexture->numMipMapLevels; i++)
        {
            int prevMipMapSideLength = mipMapSideLength;
            mipMapSideLength /=2;

            TextureData prevMipData;
            prevMipData.colorData.resize(prevMipMapSideLength*prevMipMapSideLength*prevMipMapSideLength);
            prevMipData.normalData.resize(prevMipMapSideLength*prevMipMapSideLength*prevMipMapSideLength);

            glBindTexture(GL_TEXTURE_3D, voxelTexture->colorTexture);
            glGetTexImage(GL_TEXTURE_3D, i-1, GL_RGBA, GL_UNSIGNED_BYTE, &prevMipData.colorData[0]);
            glBindTexture(GL_TEXTURE_3D, voxelTexture->normalTexture);
            glGetTexImage(GL_TEXTURE_3D, i-1, GL_RGB, GL_FLOAT, &prevMipData.normalData[0]);
           
            TextureData currMipData;
            currMipData.colorData.resize(mipMapSideLength*mipMapSideLength*mipMapSideLength);
            currMipData.normalData.resize(mipMapSideLength*mipMapSideLength*mipMapSideLength);

            for(int j = 0; j < mipMapSideLength; j++)
            for(int k = 0; k < mipMapSideLength; k++)
            for(int l = 0; l < mipMapSideLength; l++)
            {
                glm::vec4 summedColor(0);
                glm::vec3 summedNormal(0);

                glm::uvec3 index3d(j*2, k*2, l*2);  
                for(int m = 0; m < 2; m++)
                for(int n = 0; n < 2; n++)
                for(int o = 0; o < 2; o++)
                {
                    glm::uvec3 neighbor = index3d + glm::uvec3(m,n,o);
                    uint neighborIndex1d = indexConverter(prevMipMapSideLength, neighbor);
                   
                    glm::vec4 neighborColor(prevMipData.colorData[neighborIndex1d]);
                    summedColor += glm::vec4(glm::vec3(neighborColor)*(neighborColor.a/255.0f), neighborColor.a/255.0f);
                
                    glm::vec3 neighborNormal = prevMipData.normalData[neighborIndex1d];
                    summedNormal += neighborNormal;
                }

                glm::vec3 averageColor = glm::vec3(summedColor)/summedColor.a;
                glm::vec4 finalColor(averageColor, summedColor.a/8*255);

                glm::vec3 finalNormal = glm::normalize(summedNormal);

                uint index1d = indexConverter(mipMapSideLength, glm::uvec3(j,k,l));
                currMipData.colorData[index1d] = glm::u8vec4(finalColor);
                currMipData.normalData[index1d] = finalNormal;
            }

            voxelTexture->setData(currMipData, mipMapSideLength, i);
        }
    }
};