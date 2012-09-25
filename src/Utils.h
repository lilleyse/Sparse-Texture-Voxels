#pragma once

#include <glf.hpp>

typedef unsigned char uchar;
typedef unsigned int uint;

namespace Utils
{
    uint getNumMipMapLevels(uint size)
    {
        return (uint)(glm::log2(float(size)) + 1.5);
    }
}