#pragma once

#include <CL/opencl.h>
#include "Utils.h"
#include "VoxelTexture.h"


struct ColorMipmapperKernel
{
    enum params
    {
        TEXTURE_BASE,
        TEXTURE_MIPS_PBO
    };

    glm::uvec3 globalWorkSize;
    glm::uvec3 localWorkSize;
};


class OpenCLState
{
private:

    VoxelTexture* voxelTexture;

    // OpenCL stuff
    cl_platform_id clPlatform;
    cl_context clGPUContext;
    cl_device_id clDevice;
    cl_command_queue clCommandQueue;
    cl_program clProgram;
    cl_kernel clColorMipmapperKernel;
    cl_int clError;

    // GL stuff
    GLuint textureMipsPBO;

    // CL memory that interlinks with GL memory
    cl_mem clTextureBase;
    cl_mem clTextureMipsPBO;

    ColorMipmapperKernel colorMipmapperKernel;
    

    char* loadProgramSource(const char* cFilename, size_t* szFinalLength)
    {
        // locals 
        FILE* pFileStream = NULL;
        size_t szSourceLength;

        if(fopen_s(&pFileStream, cFilename, "rb") != 0)
        {
            return NULL;
        }

        // get the length of the source code
        fseek(pFileStream, 0, SEEK_END); 
        szSourceLength = ftell(pFileStream);
        fseek(pFileStream, 0, SEEK_SET); 

        // allocate a buffer for the source code string and read it in
        char* cSourceString = (char *)malloc(szSourceLength + 1); 
        if (fread((cSourceString), szSourceLength, 1, pFileStream) != 1)
        {
            fclose(pFileStream);
            free(cSourceString);
            return 0;
        }


        // close the file and return the total length of the string
        fclose(pFileStream);
        if(szFinalLength != 0)
        {
            *szFinalLength = szSourceLength;
        }
        cSourceString[szSourceLength] = '\0';

        return cSourceString;
    }

    

public:
    

    bool begin(VoxelTexture* voxelTexture)
    {
        this->voxelTexture = voxelTexture;

        std::string programName = SHADER_DIRECTORY + "mipmapper.cl";
        std::string kernelName = "colorMipmapper";

        //-------------------------------------
        // Initalize OpenCL
        //-------------------------------------

        // Get an OpenCL platform
        cl_platform_id clPlatforms[10];
        cl_uint numPlatforms;
        clError = clGetPlatformIDs(10, clPlatforms, &numPlatforms);
        if (clError != CL_SUCCESS)
            printf("could not create platform");

        // Print info about the available platforms
        for (unsigned int i = 0; i < numPlatforms; i++)
        {
            char buffer[10240];
            printf("  -- %d --\n", i);
            clGetPlatformInfo(clPlatforms[i], CL_PLATFORM_PROFILE, 10240, buffer, NULL);
            printf("  PROFILE = %s\n", buffer);
            clGetPlatformInfo(clPlatforms[i], CL_PLATFORM_VERSION, 10240, buffer, NULL);
            printf("  VERSION = %s\n", buffer);
            clGetPlatformInfo(clPlatforms[i], CL_PLATFORM_NAME, 10240, buffer, NULL);
            printf("  NAME = %s\n", buffer);
            clGetPlatformInfo(clPlatforms[i], CL_PLATFORM_VENDOR, 10240, buffer, NULL);
            printf("  VENDOR = %s\n", buffer);
            clGetPlatformInfo(clPlatforms[i], CL_PLATFORM_EXTENSIONS, 10240, buffer, NULL);
            printf("  EXTENSIONS = %s\n", buffer);
        }

        // Chose the platform that contains the AMD card
        clPlatform = clPlatforms[1];

        // Get the device - for now just assume that the device supports sharing with OpenGL
        clError = clGetDeviceIDs(clPlatform, CL_DEVICE_TYPE_GPU, 1, &clDevice, NULL);
        if (clError != CL_SUCCESS) 
            printf("could not get a GPU device on the platform");

        // Create the context, with support for sharing with OpenGL 
        cl_context_properties props[] = 
        {
            CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(), 
            CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(), 
            CL_CONTEXT_PLATFORM, (cl_context_properties)clPlatform, 
            0
        };
        clGPUContext = clCreateContext(props, 1, &clDevice, NULL, NULL, &clError);
        if (clError != CL_SUCCESS)
            printf("could not create a context");

        // Create a command-queue
        clCommandQueue = clCreateCommandQueue(clGPUContext, clDevice, 0, &clError);
        if (clError != CL_SUCCESS)
            printf("could not create command queue");

        // Load program source code
        size_t programLength;
        char* cSourceCL = loadProgramSource(programName.c_str(), &programLength);
        if(cSourceCL == NULL)
            printf("could not load program source");
  
        // Create the program
        clProgram = clCreateProgramWithSource(clGPUContext, 1, (const char **) &cSourceCL, &programLength, &clError);
        if (clError != CL_SUCCESS)
            printf("could not create program");

        // Build the program
        clError = clBuildProgram(clProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL);
        if (clError != CL_SUCCESS)
        {
            printf("could not build program");
            char cBuildLog[10240];
            clGetProgramBuildInfo(clProgram, clDevice, CL_PROGRAM_BUILD_LOG, sizeof(cBuildLog), cBuildLog, NULL);
            printf(cBuildLog);
        }

        // Create the texture 3d write kernel
        clColorMipmapperKernel = clCreateKernel(clProgram, kernelName.c_str(), &clError);
        if (clError != CL_SUCCESS)
            printf("could not create the texture 3d write kernel");

        //-----------------------------------------------------
        // Create OpenCL object from the texture base level
        //-----------------------------------------------------

        // Create CL versions of the first mipmap level of the 3D voxel texture
        clTextureBase = clCreateFromGLTexture3D(clGPUContext, CL_MEM_READ_ONLY, GL_TEXTURE_3D, 0, voxelTexture->colorTexture, &clError);
        if (clError != CL_SUCCESS)
            printf("could not create CL texture3D mip level 0 from OpenGL texture3D");

        //-----------------------------------------------------
        // Create Pixel Buffer Object for the other mip levels
        //-----------------------------------------------------
        
        glGenBuffers(1, &textureMipsPBO);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, textureMipsPBO);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, voxelTexture->totalVoxels, 0, GL_STREAM_COPY);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        clTextureMipsPBO = clCreateFromGLBuffer(clGPUContext, CL_MEM_READ_WRITE, textureMipsPBO, &clError);
        if (clError != CL_SUCCESS)
            printf("could not create CL texture pbo from OpenGL pbo");

        //-----------------------------------------------------
        // Set kernel parameters
        //-----------------------------------------------------

        clError  = clSetKernelArg(clColorMipmapperKernel, ColorMipmapperKernel::TEXTURE_BASE, sizeof(cl_mem), &clTextureBase);
        clError |= clSetKernelArg(clColorMipmapperKernel, ColorMipmapperKernel::TEXTURE_MIPS_PBO, sizeof(cl_mem), &clTextureMipsPBO);
        if (clError != CL_SUCCESS)
            printf("could not set kernel arguments");

        //-----------------------------------------------------
        // Set kernel work sizes
        //-----------------------------------------------------

        colorMipmapperKernel.globalWorkSize = glm::uvec3(voxelTexture->mipMapInfoArray[0].numVoxels/8);
        colorMipmapperKernel.localWorkSize = glm::uvec3(4,4,4);

        return true;
    }

    void display()
    {
        // Acquire GL memory
        clError  = clEnqueueAcquireGLObjects(clCommandQueue, 1, &clTextureBase, 0,0,0);
        clError |= clEnqueueAcquireGLObjects(clCommandQueue, 1, &clTextureMipsPBO, 0,0,0);
        if (clError != CL_SUCCESS)
            printf("could not acquire OpenGL memory objects");

        // Call the kernel
        clError = clEnqueueNDRangeKernel(clCommandQueue, clColorMipmapperKernel, 3, &glm::uvec3(0)[0], &colorMipmapperKernel.globalWorkSize[0], &colorMipmapperKernel.localWorkSize[0], 0,0,0);
        if (clError != CL_SUCCESS)
            printf("could not call the kernel");

        // Release GL memory
        clError  = clEnqueueReleaseGLObjects(clCommandQueue, 1, &clTextureBase, 0,0,0);
        clError |= clEnqueueReleaseGLObjects(clCommandQueue, 1, &clTextureMipsPBO, 0,0,0);
        if (clError != CL_SUCCESS)
            printf("could not release OpenGL memory objects");

        clFinish(clCommandQueue);
    }
};



