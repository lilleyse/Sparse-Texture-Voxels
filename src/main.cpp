#include "Utils.h"
#include "ShaderConstants.h"
#include "Camera.h"
#include "VoxelTextureGenerator.h"
#include "Voxelizer.h"
#include "engine/CoreEngine.h"
#include "demos/DebugDraw.h"
#include "demos/VoxelRaycaster.h"
#include "demos/VoxelConetracer.h"
#include "demos/DeferredPipeline.h"

namespace
{
    // Window
    std::string applicationName("Sparse Texture Voxels");
    glm::ivec2 windowSize(600, 400);
    glm::ivec2 openGLVersion(4, 2);
    ThirdPersonCamera* camera = new ThirdPersonCamera();
    glm::ivec2 currentMousePos;
    bool showDebugOutput = true;
    bool showFPS = true;
    bool vsync = false;
    int frameCount = 0;
    float frameTime = 0.0f;
    const float FRAME_TIME_DELTA = 0.01f;
    
    // Texture settings
    const std::string voxelTextures[] = {
        VoxelTextureGenerator::CORNELL_BOX,
        VoxelTextureGenerator::SPHERE,
        VoxelTextureGenerator::CUBE,
        DATA_DIRECTORY + "Bucky.raw",
    };

    std::string sceneFile = SCENE_DIRECTORY + "cornell.xml";
    uint voxelGridLength = 64;
    uint numMipMapLevels = 0; // If 0, then calculate the number based on the grid length
    uint currentMipMapLevel = 0;
    VoxelTextureGenerator* voxelTextureGenerator = new VoxelTextureGenerator();
    VoxelTexture* voxelTexture = new VoxelTexture();
    Voxelizer* voxelizer = new Voxelizer();
    MipMapGenerator* mipMapGenerator = new MipMapGenerator();

    
    // Demo settings
    enum DemoType {DEBUGDRAW, VOXELRAYCASTER, VOXELCONETRACER, DEFERRED_PIPELINE, MAX_DEMO_TYPES};
    DebugDraw* debugDraw = new DebugDraw();
    VoxelRaycaster* voxelRaycaster = new VoxelRaycaster();
    VoxelConetracer* voxelConetracer = new VoxelConetracer();
    DeferredPipeline* deferredPipeline = new DeferredPipeline();
    DemoType currentDemoType = DEFERRED_PIPELINE;
    bool loadAllDemos = true;

    // OpenGL stuff
    CoreEngine* coreEngine = new CoreEngine();
    FullScreenQuad* fullScreenQuad = new FullScreenQuad();
    GLuint perFrameUBO;
}

void setMipMapLevel(int level)
{
    int numMipMapLevels = voxelTexture->numMipMapLevels;
    if (level < 0) level = 0;
    if (level >= numMipMapLevels) level = numMipMapLevels - 1;
    currentMipMapLevel = level;
    
    if (loadAllDemos || currentDemoType == DEBUGDRAW)
        debugDraw->setMipMapLevel(currentMipMapLevel);
    if (loadAllDemos || currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->setMipMapLevel(currentMipMapLevel);
}

void GLFWCALL mouseMove(int x, int y)
{
    glm::ivec2 newMousePos = glm::ivec2(x,y);
    glm::ivec2 mouseDelta = newMousePos - currentMousePos; 
    currentMousePos = newMousePos;

    bool leftPress = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool rightPress = glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    bool middlePress = glfwGetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    if (leftPress || rightPress || middlePress)
    {
        float cameraDistanceFromCenter = glm::length(camera->position);
        if (leftPress)
        {
            float rotateAmount = cameraDistanceFromCenter / 200.0f;
            camera->rotate(-mouseDelta.x * rotateAmount, -mouseDelta.y * rotateAmount);
        }
        else if (rightPress)
        {
            float zoomAmount = cameraDistanceFromCenter / 200.0f;
            camera->zoom(mouseDelta.y * zoomAmount);
        }
        else if (middlePress)
        {
            float panAmount = cameraDistanceFromCenter / 500.0f;
            camera->pan(-mouseDelta.x * panAmount, mouseDelta.y * panAmount);
        }
    }
}

void GLFWCALL key(int k, int action)
{
    if (action == GLFW_RELEASE)
    {
        // Changing demo (number keys and numpad)
        if (loadAllDemos && k >= '1' && k < '1' + MAX_DEMO_TYPES)
            currentDemoType = (DemoType)((uint)k - '1');
        if (loadAllDemos && k >= GLFW_KEY_KP_1 && k < GLFW_KEY_KP_1 + MAX_DEMO_TYPES)
            currentDemoType = (DemoType)((uint)k - GLFW_KEY_KP_1);

        // Changing mip map level
        if (k == '.') setMipMapLevel((int)currentMipMapLevel + 1);
        if (k == ',') setMipMapLevel((int)currentMipMapLevel - 1);

        // Changing textures
        bool setsNextTexture = k == ';' && voxelTextureGenerator->setNextTexture();
        bool setsPreviousTexture = k == '\'' && voxelTextureGenerator->setPreviousTexture();
        if (setsNextTexture || setsPreviousTexture)
            if (loadAllDemos || currentDemoType == DEBUGDRAW)
                debugDraw->voxelTextureUpdate();
    }
}

void GLFWCALL resize(int w, int h)
{
    glViewport(0, 0, w, h);
    camera->setAspectRatio(w, h);
    windowSize = glm::ivec2(w, h);

    if (loadAllDemos || currentDemoType == DEFERRED_PIPELINE)
        deferredPipeline->resize(w, h);
}

void initGL()
{
    glewExperimental = GL_TRUE;
    glewInit();

    // Debug output
    if(showDebugOutput && Utils::OpenGL::checkExtension("GL_ARB_debug_output"))
    {
        glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC) glfwGetProcAddress("glDebugMessageControlARB");
        glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC) glfwGetProcAddress("glDebugMessageCallbackARB");
        glDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC) glfwGetProcAddress("glDebugMessageInsertARB");
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        glDebugMessageCallbackARB(&Utils::OpenGL::debugOutput, NULL);
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

void begin()
{
    initGL();

    camera->setFarNearPlanes(.01f, 100.0f);
    camera->zoom(3.0f);
    camera->lookAt = glm::vec3(0.5f);

    // set up miscellaneous things
    coreEngine->begin(sceneFile);
    fullScreenQuad->begin();
    voxelTexture->begin(voxelGridLength, numMipMapLevels);
    mipMapGenerator->begin(voxelTexture, fullScreenQuad);
    voxelTextureGenerator->begin(voxelTexture, mipMapGenerator);
    
    // voxelize from the triangle scene. Do this first because the 3d texture starts as empty
    voxelizer->begin(voxelTexture, coreEngine, perFrameUBO);
    voxelizer->voxelizeScene();
    voxelTextureGenerator->createTextureFromVoxelTexture(sceneFile);

    // create procedural textures
    uint numInitialTextures = sizeof(voxelTextures) / sizeof(voxelTextures[0]);
    for (uint i = 0; i < numInitialTextures; i++)
        voxelTextureGenerator->createTexture(voxelTextures[i]);    

    // set the active texture to the triangle scene
    voxelTextureGenerator->setTexture(sceneFile);

    

    // init demos
    if (loadAllDemos || currentDemoType == DEBUGDRAW) 
        debugDraw->begin(voxelTexture);
    if (loadAllDemos || currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->begin(voxelTexture, fullScreenQuad);
    if (loadAllDemos || currentDemoType == VOXELCONETRACER)
        voxelConetracer->begin(voxelTexture, fullScreenQuad);
    if (loadAllDemos || currentDemoType == DEFERRED_PIPELINE)
        deferredPipeline->begin(voxelTexture, fullScreenQuad, coreEngine);

    setMipMapLevel(currentMipMapLevel);
}

void display()
{
    // Update the scene
    //coreEngine->scene->objects[0]->translate(glm::vec3(0,.001,0));
    //coreEngine->updateScene();
    //voxelizer->voxelizeScene();

    // Basic GL stuff
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    float clearColor[4] = {0.0f,0.0f,0.0f,1.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor);
    float clearDepth = 1.0f;
    glClearBufferfv(GL_DEPTH, 0, &clearDepth);

    // Update the per frame UBO
    PerFrameUBO perFrame;
    perFrame.uViewProjection = camera->createProjectionMatrix() * camera->createViewMatrix();    
    perFrame.uCamLookAt = camera->lookAt;
    perFrame.uCamPos = camera->position;
    perFrame.uCamUp = camera->upDir;
    perFrame.uResolution = glm::vec2(windowSize);
    perFrame.uAspect = (float)windowSize.x/windowSize.y;
    perFrame.uTime = frameTime;
    perFrame.uFOV = camera->fieldOfView;
    perFrame.uTextureRes = (float)voxelTexture->voxelGridLength;
    perFrame.uNumMips = (float)voxelTexture->numMipMapLevels;

    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), &perFrame);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //mipMapGenerator->generateMipMapGPU();

    // Display demo
    if (currentDemoType == DEBUGDRAW)
        debugDraw->display();
    else if (currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->display(); 
    else if (currentDemoType == VOXELCONETRACER)  
        voxelConetracer->display();
    else if (currentDemoType == DEFERRED_PIPELINE)
        deferredPipeline->display();

}

void displayFPS()
{
    frameCount += 1;
    double currentTime = glfwGetTime();
    if(currentTime >= 1.0)
    {	
        std::ostringstream ss;
		ss << applicationName << " (fps: " << (frameCount/currentTime) << " )";
        glfwSetWindowTitle(ss.str().c_str());
        glfwSetTime(0.0);
        frameCount = 0;
    }
}
int main(int argc, char* argv[])
{
    glfwInit();
    glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, openGLVersion.x);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, openGLVersion.y);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwOpenWindow(windowSize.x, windowSize.y, 0, 0, 0, 0, 24, 0, GLFW_WINDOW);

    begin();
    glfwSetWindowTitle(applicationName.c_str());
    glfwSetWindowSizeCallback(resize);
    glfwSetMousePosCallback(mouseMove);
    glfwSetKeyCallback(key);
    glfwEnable(GLFW_KEY_REPEAT);
    glfwEnable(GLFW_STICKY_KEYS);
    glfwSwapInterval(vsync ? 1 : 0);
    glfwSetTime(0.0);

    bool running = true;
    do
    {
        display();
        glfwSwapBuffers();
        frameTime += FRAME_TIME_DELTA;
        if (showFPS) displayFPS();
        running = !glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED);
    }
    while(running);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
