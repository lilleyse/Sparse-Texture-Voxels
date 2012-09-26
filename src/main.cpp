#include <glf.hpp>
#include "ShaderConstants.h"
#include "Camera.h"
#include "Utils.h"
#include "demos/DebugDraw.h"
#include "demos/VoxelRaycaster.h"
#include "VoxelTextureGenerator.h"

enum DemoType {DEBUGDRAW, VOXELRAYCASTER, MAX_DEMO_TYPES};

namespace
{
    std::string const SAMPLE_NAME("Sparse Texture Voxels");
    const int SAMPLE_SIZE_WIDTH(600);
    const int SAMPLE_SIZE_HEIGHT(400);
    const int SAMPLE_MAJOR_VERSION(3);
    const int SAMPLE_MINOR_VERSION(3);
    
    // Window and updating
    glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));
    GLuint perFrameUBO;
    ThirdPersonCamera camera;
    bool showDebugOutput = false;
    float frameTime = 0.0f;
    const float FRAME_TIME_DELTA = 0.01f;
    bool showFPS = true;
    Utils::Framerate fpsHandler;
    
    // Texture settings
    VoxelTextureGenerator voxelTextureGenerator;
    const std::string initialTextures[] = {"data/Bucky.raw"};
    bool loadMultipleTextures = true;
    uint voxelGridLength = 32;
    uint numMipMapLevels;
    uint currentMipMapLevel;

    // Demo settings
    DebugDraw debugDraw;
    VoxelRaycaster voxelRaycaster;
    DemoType currentDemoType = DEBUGDRAW;
    bool loadAllDemos = true;
}

void setMipMapLevel(int level)
{
    if (level < 0) level = 0;
    if (level >= (int)numMipMapLevels) level = numMipMapLevels - 1;
    if (level == currentMipMapLevel) return;
    currentMipMapLevel = level;
    if (loadAllDemos || currentDemoType == DEBUGDRAW)
        debugDraw.setMipMapLevel(currentMipMapLevel);
    if (loadAllDemos || currentDemoType == VOXELRAYCASTER)
        voxelRaycaster.setMipMapLevel(currentMipMapLevel);
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
    float zoomAmount = cameraDistanceFromCenter / 200.0f;
    float panAmount = cameraDistanceFromCenter / 500.0f;

    camera.rotate(-Window.LeftMouseDelta.x * rotateAmount, -Window.LeftMouseDelta.y * rotateAmount);
    camera.zoom(Window.RightMouseDelta.y * zoomAmount);
    camera.pan(-Window.MiddleMouseDelta.x * panAmount, Window.MiddleMouseDelta.y * panAmount);
}

void keyboardEvent(uchar keyCode)
{
    // Changing demo
    if (loadAllDemos && keyCode >= 49 && keyCode < 49 + MAX_DEMO_TYPES) 
        currentDemoType = (DemoType)((uint)keyCode - 49);

    // Changing mip map level
    if (keyCode == 46) setMipMapLevel((int)currentMipMapLevel + 1);
    if (keyCode == 44) setMipMapLevel((int)currentMipMapLevel - 1);

    // Changing textures
    bool setsNextTexture = keyCode == 59 && voxelTextureGenerator.setNextTexture();
    bool setsPreviousTexture = keyCode == 39 && voxelTextureGenerator.setPreviousTexture();
    if (setsNextTexture || setsPreviousTexture)
        if (loadAllDemos || currentDemoType == DEBUGDRAW)
            debugDraw.createCubesFromVoxels(voxelTextureGenerator.getVoxelTexture());
}



bool begin()
{
    initGL();

    camera.setFarNearPlanes(.01f, 100.0f);
    camera.lookAt = glm::vec3(0.5f);
    camera.zoom(-2);

    // all process, nothing interesting here
    voxelTextureGenerator.begin(voxelGridLength, loadMultipleTextures);
    uint numInitialTextures = sizeof(initialTextures) / sizeof(initialTextures[0]);
    for (uint i = 0; i < numInitialTextures; i++)
        voxelTextureGenerator.createTexture(initialTextures[i]);
    voxelTextureGenerator.createAllPresets();
    voxelTextureGenerator.setTexture(0);
    VoxelTexture* voxelTexture = voxelTextureGenerator.getVoxelTexture();
    
    // init demos
    if (loadAllDemos || currentDemoType == DEBUGDRAW) 
        debugDraw.begin(voxelTexture);
    if (loadAllDemos || currentDemoType == VOXELRAYCASTER)
        voxelRaycaster.begin();
    
    // initial mip-map setting
    numMipMapLevels = voxelTexture->numMipMapLevels;
    currentMipMapLevel = UINT_MAX;
    setMipMapLevel(currentMipMapLevel);

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
    perFrame.uResolution = glm::uvec2(Window.Size.x, Window.Size.y);
    perFrame.uTime = frameTime;
    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), &perFrame);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Display demo
    if (currentDemoType == DEBUGDRAW)
        debugDraw.display();
    else if (currentDemoType == VOXELRAYCASTER)
        voxelRaycaster.display(); 

    glf::swapBuffers();

    frameTime += FRAME_TIME_DELTA;
   
    if(showFPS)
        fpsHandler.display();
}

int main(int argc, char* argv[])
{
    return glf::run(
        argc, argv,
        glm::ivec2(::SAMPLE_SIZE_WIDTH, ::SAMPLE_SIZE_HEIGHT),
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB, ::SAMPLE_MAJOR_VERSION,
        ::SAMPLE_MINOR_VERSION);
}
