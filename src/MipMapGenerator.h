#pragma once
#include <glf.hpp>
#include <CL/opencl.h>
#include <sstream>

#include "ShaderConstants.h"
#include "Utils.h"
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
        glBindTexture(GL_TEXTURE_3D, voxelTexture->textureGL);

        int mipMapSideLength = voxelTexture->voxelGridLength;
        for(int i = 1; i < voxelTexture->numMipMapLevels; i++)
        {
            int prevMipMapSideLength = mipMapSideLength;
            mipMapSideLength /=2;

            std::vector<glm::u8vec4> prevMipData(prevMipMapSideLength*prevMipMapSideLength*prevMipMapSideLength);
            glGetTexImage(GL_TEXTURE_3D, i-1, GL_RGBA, GL_UNSIGNED_BYTE, &prevMipData[0]);

           
            std::vector<glm::u8vec4> currMipData(mipMapSideLength*mipMapSideLength*mipMapSideLength);

            for(int j = 0; j < mipMapSideLength; j++)
            for(int k = 0; k < mipMapSideLength; k++)
            for(int l = 0; l < mipMapSideLength; l++)
            {
                glm::vec4 summedColor(0,0,0,0);

                glm::uvec3 index3d(j*2, k*2, l*2);  
                for(int m = 0; m < 2; m++)
                for(int n = 0; n < 2; n++)
                for(int o = 0; o < 2; o++)
                {
                    glm::uvec3 neighbor = index3d + glm::uvec3(m,n,o);
                    uint neighborIndex1d = indexConverter(prevMipMapSideLength, neighbor);
                    glm::vec4 neighborColor(prevMipData[neighborIndex1d]);
                    summedColor += glm::vec4(glm::vec3(neighborColor)*(neighborColor.a/255.0f), neighborColor.a/255.0f);
                }

                glm::vec3 averageColor = glm::vec3(summedColor)/summedColor.a;
                glm::vec4 finalColor(averageColor, summedColor.a/8*255);
                uint index1d = indexConverter(mipMapSideLength, glm::uvec3(j,k,l));
                currMipData[index1d] = glm::u8vec4(finalColor);
            }

            glTexSubImage3D(GL_TEXTURE_3D, i, 0, 0, 0, mipMapSideLength, mipMapSideLength, mipMapSideLength, GL_RGBA, GL_UNSIGNED_BYTE, &currMipData[0]);
        }
    }
};