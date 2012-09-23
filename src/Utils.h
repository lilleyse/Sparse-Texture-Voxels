#pragma once

#include <glf.hpp>

namespace Utils
{
    unsigned int getNumMipMapLevels(unsigned int size)
    {
        return (unsigned int)(glm::log2(float(size)) + 1.5);
    }
}