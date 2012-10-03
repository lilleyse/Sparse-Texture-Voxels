#pragma once

#include <glf.hpp>
#include <istream>
#include <sstream>
#include <iterator>
#include <iostream>

typedef unsigned char uchar;
typedef unsigned int uint;

const std::string SHADER_DIRECTORY = std::string("src/shaders/");
const std::string SCENE_DIRECTORY = std::string("data/scenes/");
const std::string IMAGE_DIRECTORY = std::string("data/images/");
const std::string MESH_DIRECTORY = std::string("data/meshes/");

namespace Utils
{
    struct Framerate
    {
	    std::clock_t start;
	    int frameCount;

	    Framerate()
	    {
		    startTimer();
	    }
	    double getDuration()
	    {
		    return ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
	    }
	    void startTimer()
	    {
		    frameCount = 0;
		    start = std::clock();
	    }

	    void display()
	    {
		    frameCount++;
		    double duration = getDuration();
		    if(duration >= 1.0)
		    {
			    double fps = frameCount / duration;
			    printf("framerate: %f\n", fps);
			    startTimer();
		    }
	    }
	
    };

    int roundToNextPowerOf2(int x)
    {
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return ++x;
    }

    int roundToNextMultiple(int numToRound, int multiple)
    {
        return numToRound == 0 ? 0 : ((numToRound - 1) / multiple + 1) * multiple;
    }


    void printMatrix(glm::mat4& matrix)
    {
        std::cout << "==========PRINTING MATRIX==========" << std::endl;
        std::cout << matrix[0].x << " " << matrix[1].x << " " << matrix[2].x << " " << matrix[3].x << std::endl;
        std::cout << matrix[0].y << " " << matrix[1].y << " " << matrix[2].y << " " << matrix[3].y << std::endl;
        std::cout << matrix[0].z << " " << matrix[1].z << " " << matrix[2].z << " " << matrix[3].z << std::endl;
        std::cout << matrix[0].w << " " << matrix[1].w << " " << matrix[2].w << " " << matrix[3].w << std::endl;
    }
    void printVec3(glm::vec3& vector)
    {
        std::cout << vector[0] << " " << vector[1] << " " << vector[2] << std::endl;
    }
    void printVec4(glm::vec4& vector)
    {
        std::cout << vector[0] << " " << vector[1] << " " << vector[2] << " " << vector[3] << std::endl;
    }

    void printQuat(glm::fquat& quaternion)
    {
        std::cout << quaternion[0] << " " << quaternion[1] << " " << quaternion[2] << " " << quaternion[3] << std::endl;
    }

    std::vector<std::string> parseSpaceSeparatedString(std::string& configData)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(configData);
        std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter<std::vector<std::string> >(tokens));
        return tokens;
    }
}