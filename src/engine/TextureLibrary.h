#pragma once 

#include "../Utils.h"
#include "ShaderLibrary.h"
#include "RenderData.h"

struct TextureLibrary
{

    struct TextureArray
    {
        gli::texture2D::format_type format;
        glm::uvec2 resolution;
        uint numMipMaps;
        std::vector<gli::texture2D> textures;

        bool operator==(const TextureArray& other) const
        {
            return format == other.format && resolution == other.resolution && numMipMaps == other.numMipMaps;
        }
    };

    std::vector<TextureArray> textureArrays;

    // Meta information about a particular 2d texture
    struct TextureMetaData
    {
        uint textureArrayID;
        uint indexInArray;
    };

    std::map<std::string, TextureMetaData> textureNames;

    TextureMetaData addTexture(std::string& textureName)
    {
        // Check if this texture name has already been added
        std::map<std::string, TextureMetaData>::iterator foundName = textureNames.find(textureName);
        if(foundName == textureNames.end())
        {
            // This texture name has not been added yet

            // Load the texture, get it's format, resolution, and mipmaps
            gli::texture2D loadedTexture = gli::load(IMAGE_DIRECTORY + textureName);
            TextureArray textureArrayTest;
            textureArrayTest.format = loadedTexture.format();
            textureArrayTest.numMipMaps = loadedTexture.levels();
            textureArrayTest.resolution = loadedTexture[0].dimensions();
           
            // See if a texture array already exists with these properties
            std::vector<TextureArray>::iterator foundTextureArray = std::find(textureArrays.begin(), textureArrays.end(), textureArrayTest);

            if(foundTextureArray == textureArrays.end())
            {
                // A suitable texture array was not found

                // Create the texture meta data 
                TextureMetaData textureMetaData;
                textureMetaData.textureArrayID = textureArrays.size();
                textureMetaData.indexInArray = 0;
                textureNames[textureName] = textureMetaData;

                // Create a new texture array
                textureArrayTest.textures.push_back(loadedTexture);
                textureArrays.push_back(textureArrayTest);

                return textureMetaData;
            }
            else
            {
                // A suitable texture array was found

                // Create the texture meta data 
                TextureMetaData textureMetaData;
                textureMetaData.textureArrayID = foundTextureArray - textureArrays.begin();
                textureMetaData.indexInArray = foundTextureArray->textures.size();
                textureNames[textureName] = textureMetaData;

                // Add the texture to the texture array
                foundTextureArray->textures.push_back(loadedTexture);

                return textureMetaData;
            }
        }
        else
        {
            // Since the texture has already been added, just return the texture meta data
            TextureMetaData textureMetaData = foundName->second;
            return textureMetaData;
        }
    }


    void commitToGL()
    {
        uint numTextureArrays = textureArrays.size();
        if(numTextureArrays > MAX_TEXTURE_ARRAYS)
        {
            printf("too many texture arrays! Change the MAX_TEXTURE_ARRAYS constant");
            return;
        }

        // Generate the GL texture arrays
        GLuint* textureArraysGL = new GLuint[numTextureArrays];
        glGenTextures(numTextureArrays, textureArraysGL);

		// Create the texture sampler
		GLuint textureSampler;
		glGenSamplers(1, &textureSampler);
        glBindSampler(NON_USED_TEXTURE, textureSampler);
        glSamplerParameteri(textureSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glSamplerParameteri(textureSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Loop over the texture arrays
        for(uint i = 0; i < textureArrays.size(); i++)
        {
            TextureArray& textureArray = textureArrays[i];

            // Set active texture and bind texture
            glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING + i);
            glBindTexture(GL_TEXTURE_2D_ARRAY, textureArraysGL[i]);
			glBindSampler(DIFFUSE_TEXTURE_ARRAY_SAMPLER_BINDING + i, textureSampler);

            // Get the gli format to a proper opengl format
            GLuint textureInternalformat;
            GLuint textureFormat;
            GLuint textureType;

            // Only supports RGB8 and RGBA8 right now
            if(textureArray.format == gli::RGB8U)
            {
                textureInternalformat = GL_RGB8;
                textureFormat = GL_BGR;
                textureType = GL_UNSIGNED_BYTE;
            }
            else if(textureArray.format == gli::RGBA8U)
            {
                textureInternalformat = GL_RGBA8;
                textureFormat = GL_BGRA;
                textureType = GL_UNSIGNED_BYTE;
            }

            // Create the texture
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, textureArray.numMipMaps, textureInternalformat, textureArray.resolution.x, textureArray.resolution.y, textureArray.textures.size());
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, textureArray.numMipMaps-1);

            // Fill in the data for each texture in the texture array
            for(uint j = 0; j < textureArray.textures.size(); j++)
            {
                gli::texture2D& textureData = textureArray.textures[j];

                // Fill in each mipmap level
                for(gli::texture2D::level_type level = 0; level < textureArray.numMipMaps; level++)
                {
                    glTexSubImage3D(
                        GL_TEXTURE_2D_ARRAY, 
                        GLint(level), 
                        0, 0, j,
                        GLsizei(textureData[level].dimensions().x), 
                        GLsizei(textureData[level].dimensions().y),
                        1,
                        textureFormat,
                        textureType, 
                        textureData[level].data());
                }
            }
        }
    }
};