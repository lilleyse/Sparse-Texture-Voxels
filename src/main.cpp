#include <glf.hpp>
#include "Utils.h"
#include "ShaderConstants.h"
#include "Camera.h"
#include "DebugDraw.h"
#include "VoxelRaycaster.h"
#include "VoxelConetracer.h"
#include "MipMapGenerator.h"
#include "LoadTextureFile.h"

enum DemoType {DEBUGDRAW, VOXELRAYCASTER, VOXELCONETRACER, NONE};

namespace
{
    std::string const SAMPLE_NAME("Sparse Texture Voxels");
    const int SAMPLE_SIZE_WIDTH(600);
    const int SAMPLE_SIZE_HEIGHT(400);
    const int SAMPLE_MAJOR_VERSION(3);
    const int SAMPLE_MINOR_VERSION(3);

    glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));
    bool showDebugOutput = false;

    uint voxelGridLength = 64;
    GLuint voxelTexture;
    GLuint perFrameUBO;

    ThirdPersonCamera camera;

    MipMapGenerator mipMapGenerator;

    //Demo Types
    DebugDraw debugDraw;
    VoxelRaycaster voxelRaycaster;
    VoxelConetracer voxelConetracer;
    DemoType currentDemo = VOXELCONETRACER;
    bool loadAllDemos = false;
    
    float frameTime = 0.0f;
    const float FRAME_TIME_DELTA = 0.01f;
}

void createVoxelTextureFromRaw(uchar* buffer, uint width, uint height, uint depth)
{
    // Create a dense 3D texture
    int numMipMapLevels = Utils::getNumMipMapLevels(width); // assuming cube

    glGenTextures(1, &voxelTexture);
    glActiveTexture(GL_TEXTURE0 + VOXEL_TEXTURE_3D_BINDING);
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_R8, width, height, depth);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, zeroes);

    // Fill entire texture (first mipmap level)
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, width, height, depth, GL_RED, GL_UNSIGNED_BYTE, buffer);

    // Generate mipmaps automatically
    //mipMapGenerator.generateMipMapCPU(voxelTexture, voxelGridLength, numMipMapLevels);
    glGenerateMipmap(GL_TEXTURE_3D);
}
void createVoxelTexture()
{
    // Create a dense 3D texture
    int numMipMapLevels = Utils::getNumMipMapLevels(voxelGridLength);
    glGenTextures(1, &voxelTexture);
    glActiveTexture(GL_TEXTURE0 + VOXEL_TEXTURE_3D_BINDING);
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8, voxelGridLength, voxelGridLength, voxelGridLength);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, zeroes);

    // Create the voxel data
    std::vector<glm::u8vec4> textureData(voxelGridLength*voxelGridLength*voxelGridLength);

    uint half = voxelGridLength / 2;
    uint textureIndex = 0;
    for(uint i = 0; i < voxelGridLength; i++)
    for(uint j = 0; j < voxelGridLength; j++)
    for(uint k = 0; k < voxelGridLength; k++) 
    {
        if (i<half && j<half && k<half)
            textureData[textureIndex] = glm::u8vec4(255,0,0,127);
        else if (i>=half && j<half && k<half)
            textureData[textureIndex] = glm::u8vec4(0,255,0,127);
        else if (i<half && j>=half && k<half)
            textureData[textureIndex] = glm::u8vec4(0,0,255,127);
        else if (i>=half && j>=half && k<half)
            textureData[textureIndex] = glm::u8vec4(255,255,255,127);
        else
            textureData[textureIndex] = glm::u8vec4(127,127,127,127);

        textureIndex++;
    }

    // Fill entire texture (first mipmap level)
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, voxelGridLength, voxelGridLength, voxelGridLength, GL_RGBA, GL_UNSIGNED_BYTE, &textureData[0]);

    // Generate mipmaps automatically
    mipMapGenerator.generateMipMapCPU(voxelTexture, voxelGridLength, numMipMapLevels);
}
void initGL()
{
    // Debug output
    if(showDebugOutput && glf::checkExtension("GL_ARB_debug_output"))
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        glDebugMessageCallbackARB(&glf::debugOutput, NULL);
    }
    else
    {
        printf("debug output extension not found");
    }
    
    // Create per frame uniform buffer object
    glGenBuffers(1, &perFrameUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PerFrameUBO), NULL, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, PER_FRAME_UBO_BINDING, perFrameUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Backface culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
}

void mouseEvent()
{
    float cameraDistanceFromCenter = glm::length(camera.position);

    float rotateAmount = cameraDistanceFromCenter / 200.0f;
    camera.rotate(-Window.LeftMouseDelta.x * rotateAmount, -Window.LeftMouseDelta.y * rotateAmount);
    
    float zoomAmount = cameraDistanceFromCenter / 100.0f;
    camera.zoom(Window.RightMouseDelta.y * zoomAmount);

    float panAmount = cameraDistanceFromCenter / 500.0f;
    camera.pan(-Window.MiddleMouseDelta.x * panAmount, Window.MiddleMouseDelta.y * panAmount);
}

void keyboardEvent(uchar keyCode)
{
    if (loadAllDemos && keyCode >= 49 && keyCode < 49 + NONE) 
    {
        currentDemo = (DemoType)((uint)keyCode - 49);
    }
    switch (currentDemo) 
    {
        case DEBUGDRAW:
            debugDraw.keyboardEvent(keyCode);
            break;
        case VOXELRAYCASTER:
            voxelRaycaster.keyboardEvent(keyCode);
            break;
        case VOXELCONETRACER:
            voxelConetracer.keyboardEvent(keyCode);
            break;
    }   
}

bool begin()
{
    initGL();
    //createVoxelTexture();
    
    {
        // hardcoded
        uint width, height, depth;
        width = height = depth = 32;
        uint channels = 1;

        uchar* buffer = new uchar[width*height*depth*channels];

        LoadTextureFile::LoadRaw("data/Bucky.raw", width, height, depth, channels, buffer);

        createVoxelTextureFromRaw(buffer, width, height, depth);
    }
    
    camera.setFarNearPlanes(.01f, 100.0f);
    camera.lookAt = glm::vec3(0.5f);
    //camera.zoom(-2);

    if (loadAllDemos || currentDemo == DEBUGDRAW) 
    {
        debugDraw.begin(voxelTexture, voxelGridLength);
    }
    if (loadAllDemos || currentDemo == VOXELRAYCASTER)
    {
        voxelRaycaster.begin();
    }
    if (loadAllDemos || currentDemo == VOXELCONETRACER)
    {
        voxelConetracer.begin();
    }

    return true;
}

bool end()
{
    return true;
}

void display()
{
    // Basic GL stuff
    camera.setAspectRatio(Window.Size.x, Window.Size.y);
    glViewport(0, 0, Window.Size.x, Window.Size.y);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    float clearColor[4] = {0.0f,0.0f,0.0f,1.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor);
    float clearDepth = 1.0f;
    glClearBufferfv(GL_DEPTH, 0, &clearDepth);

    // Update the per frame UBO
    PerFrameUBO perFrame;
    perFrame.viewProjection = camera.createProjectionMatrix() * camera.createViewMatrix();
    perFrame.uCamLookAt = camera.lookAt;
    perFrame.uCamPosition = camera.position;
    perFrame.uCamUp = camera.upDir;
    perFrame.uResolution = glm::vec2(Window.Size.x, Window.Size.y);
    perFrame.uTime = frameTime;
    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), &perFrame);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Display demo
    switch (currentDemo) 
    {
        case DEBUGDRAW:
            debugDraw.display(); 
            break;
        case VOXELRAYCASTER:
            voxelRaycaster.display(); 
            break;
        case VOXELCONETRACER:
            voxelConetracer.display(); 
            break;
    }  

    glf::swapBuffers();
    frameTime += FRAME_TIME_DELTA;
}

int main(int argc, char* argv[])
{
    return glf::run(
        argc, argv,
        glm::ivec2(::SAMPLE_SIZE_WIDTH, ::SAMPLE_SIZE_HEIGHT),
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB, ::SAMPLE_MAJOR_VERSION,
        ::SAMPLE_MINOR_VERSION);
}
