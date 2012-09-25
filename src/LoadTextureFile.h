#pragma once

#include <iostream>
#include <fstream>
using namespace std;

typedef unsigned char uchar;
typedef unsigned int uint;

namespace LoadTextureFile {

    void LoadRaw(
        char* filename, 
        uint width, uint height, uint depth, uint channels,
        uchar* buffer) 
    {
        // open file
        ifstream file(filename, ios::in|ios::binary);
        if (!file) {
            printf("file open error");
            return;
        }

        file.read((char*)buffer, width*height*depth*channels);
    }

}   // namespace