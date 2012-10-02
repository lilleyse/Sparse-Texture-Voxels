#include <glf.hpp>
#include "Utils.h"
#include "ShaderConstants.h"
#include "Camera.h"
#include "Utils.h"
#include "demos/DebugDraw.h"
#include "demos/VoxelRaycaster.h"
#include "demos/VoxelConetracer.h"
#include "demos/DeferredPipeline.h"
#include "VoxelTextureGenerator.h"

enum DemoType {DEBUGDRAW, VOXELRAYCASTER, VOXELCONETRACER, DEFERRED_PIPELINE, MAX_DEMO_TYPES};

namespace
{
    std::string const SAMPLE_NAME("Sparse Texture Voxels");
    const int SAMPLE_SIZE_WIDTH(600);
    const int SAMPLE_SIZE_HEIGHT(400);
    const int SAMPLE_MAJOR_VERSION(3);
    const int SAMPLE_MINOR_VERSION(3);
    
    // Window and updating
    glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));
    ThirdPersonCamera camera;
    bool showDebugOutput = false;
    float frameTime = 0.0f;
    const float FRAME_TIME_DELTA = 0.01f;
    bool showFPS = true;
    Utils::Framerate fpsHandler;
    
    // Texture settings
    VoxelTextureGenerator voxelTextureGenerator;
    VoxelTexture* voxelTexture = new VoxelTexture();
    uint currentMipMapLevel = UINT_MAX;
    uint voxelGridLength = 32;
    const std::string voxelTextures[] = {
        VoxelTextureGenerator::CORNELL_BOX,
        VoxelTextureGenerator::SPHERE,
        VoxelTextureGenerator::CUBE,
        "data/Bucky.raw",
    };
    
    // Demo settings
    DebugDraw* debugDraw = new DebugDraw();
    VoxelRaycaster* voxelRaycaster = new VoxelRaycaster();
    VoxelConetracer* voxelConetracer = new VoxelConetracer();
    DeferredPipeline* deferredPipeline = new DeferredPipeline();
    DemoType currentDemoType = DEFERRED_PIPELINE;
    bool loadAllDemos = true;

    // OpenGL stuff
    FullScreenQuad* fullScreenQuad = new FullScreenQuad();
    GLuint perFrameUBO;
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
    else printf("debug output extension not found");
    
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

void setMipMapLevel(int level)
{
    int numMipMapLevels = voxelTexture->numMipMapLevels;
    if (level < 0) level = 0;
    if (level >= numMipMapLevels) level = numMipMapLevels - 1;
    if (level == currentMipMapLevel) return;
    currentMipMapLevel = level;
    
    if (loadAllDemos || currentDemoType == DEBUGDRAW || currentDemoType == DEFERRED_PIPELINE)
        debugDraw->setMipMapLevel(currentMipMapLevel);
    if (loadAllDemos || currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->setMipMapLevel(currentMipMapLevel);
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
    {
        if (loadAllDemos || currentDemoType == DEBUGDRAW || currentDemoType == DEFERRED_PIPELINE)
            debugDraw->voxelTextureUpdate();
    }
}

bool begin()
{
    initGL();

    camera.setFarNearPlanes(.01f, 100.0f);
    camera.zoom(-2);
    camera.lookAt = glm::vec3(0.5f);

    // set up full screen quad and voxel textures
    fullScreenQuad->begin();
    voxelTextureGenerator.begin(voxelTexture, voxelGridLength);
    uint numInitialTextures = sizeof(voxelTextures) / sizeof(voxelTextures[0]);
    for (uint i = 0; i < numInitialTextures; i++)
        voxelTextureGenerator.createTexture(voxelTextures[i]);
    voxelTextureGenerator.setTexture(voxelTextures[0]);
    
    // init demos
    if (loadAllDemos || currentDemoType == DEBUGDRAW || currentDemoType == DEFERRED_PIPELINE) 
        debugDraw->begin(voxelTexture);
    if (loadAllDemos || currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->begin(voxelTexture, fullScreenQuad);
    if (loadAllDemos || currentDemoType == VOXELCONETRACER)
        voxelConetracer->begin(voxelTexture, fullScreenQuad);
    if (loadAllDemos || currentDemoType == DEFERRED_PIPELINE)
    {
        deferredPipeline->begin(voxelTexture, fullScreenQuad, debugDraw);
        deferredPipeline->resize(Window.Size.x, Window.Size.y);
    }

    setMipMapLevel(0);

    return true;
}

bool end()
{
    return true;
}

void resize(int w, int h)
{
    glViewport(0, 0, w, h);
    camera.setAspectRatio(w, h);

    if (loadAllDemos || currentDemoType == DEFERRED_PIPELINE)
        deferredPipeline->resize(w, h);
}

void display()
{
    // Basic GL stuff
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    float clearColor[4] = {0.0f,0.0f,0.0f,1.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor);
    float clearDepth = 1.0f;
    glClearBufferfv(GL_DEPTH, 0, &clearDepth);

    // Update the per frame UBO
    PerFrameUBO perFrame;
    perFrame.viewProjection = camera.createProjectionMatrix() * camera.createViewMatrix();    
    perFrame.uCamLookAt = camera.lookAt;
    perFrame.uCamPos = camera.position;
    perFrame.uCamUp = camera.upDir;
    perFrame.uResolution = glm::vec2(Window.Size.x, Window.Size.y);
    perFrame.uAspect = (float)Window.Size.x/Window.Size.y;
    perFrame.uTime = frameTime;
    perFrame.uFOV = camera.fieldOfView;
    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), &perFrame);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Display demo
    if (currentDemoType == DEBUGDRAW)
        debugDraw->display();
    else if (currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->display(); 
    else if (currentDemoType == VOXELCONETRACER)  
        voxelConetracer->display();
    else if (currentDemoType == DEFERRED_PIPELINE)
        deferredPipeline->display();

    // Update
    glf::swapBuffers();
    frameTime += FRAME_TIME_DELTA;
    if(showFPS) fpsHandler.display();
}

int main(int argc, char* argv[])
{
    return glf::run(
        argc, argv,
        glm::ivec2(::SAMPLE_SIZE_WIDTH, ::SAMPLE_SIZE_HEIGHT),
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB, ::SAMPLE_MAJOR_VERSION,
        ::SAMPLE_MINOR_VERSION);
}
