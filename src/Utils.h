#pragma once

#include <glf.hpp>

typedef unsigned char uchar;
typedef unsigned int uint;

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
}