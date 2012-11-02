#pragma once
#include "Utils.h"
#include "ShaderConstants.h"

struct NoiseTexture
{
    uint textureSize;
    GLuint noiseTexture;

    void begin()
    {
        this->textureSize = 256;
        uint numValues = textureSize * textureSize;
        std::vector<float> randomValues;
        randomValues.resize(numValues);
        for (uint i = 0; i < numValues; i++)
            randomValues[i] = glm::compRand1<float>();

        glActiveTexture(GL_TEXTURE0 + NOISE_TEXTURE_2D_BINDING);
        glGenTextures(1, &noiseTexture);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, textureSize, textureSize);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureSize, textureSize, GL_RED, GL_FLOAT, &randomValues[0]);
    }
};