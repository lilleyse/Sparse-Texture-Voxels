#include "Utils.h"
#include "ShaderConstants.h"
#include "Camera.h"
#include "VoxelTextureGenerator.h"

#include "demos/DebugDraw.h"
#include "demos/VoxelRaycaster.h"
#include "demos/VoxelConetracer.h"
#include "demos/DeferredPipeline.h"

enum DemoType {DEBUGDRAW, VOXELRAYCASTER, VOXELCONETRACER, DEFERRED_PIPELINE, MAX_DEMO_TYPES};

namespace
{
    // Window
    std::string applicationName("Sparse Texture Voxels");
    glm::ivec2 windowSize(600, 400);
    glm::ivec2 openGLVersion(4, 2);
    ThirdPersonCamera camera;
    glm::ivec2 currentMousePos;
    glm::ivec2 oldMousePos;
    bool showDebugOutput = false;
    bool showFPS = true;
    bool vsync = false;
    float frameTime = 0.0f;
    const float FRAME_TIME_DELTA = 0.01f;
    
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

void GLFWCALL mouseMove(int x, int y)
{
    oldMousePos = currentMousePos;
    currentMousePos = glm::ivec2(x,y);

    bool leftPress = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool rightPress = glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    bool middlePress = glfwGetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    if (leftPress || rightPress || middlePress)
    {
        glm::ivec2 mouseDelta = currentMousePos - oldMousePos;
        float cameraDistanceFromCenter = glm::length(camera.position);
        if (leftPress)
        {
            float rotateAmount = cameraDistanceFromCenter / 200.0f;
            camera.rotate(-mouseDelta.x * rotateAmount, -mouseDelta.y * rotateAmount);
        }
        else if (rightPress)
        {
            float zoomAmount = cameraDistanceFromCenter / 200.0f;
            camera.zoom(mouseDelta.y * zoomAmount);
        }
        else if (middlePress)
        {
            float panAmount = cameraDistanceFromCenter / 500.0f;
            camera.pan(-mouseDelta.x * panAmount, mouseDelta.y * panAmount);
        }
    }
}

void GLFWCALL key(int k, int action)
{
    if (action == GLFW_RELEASE)
    {
        // Changing demo
        std::cout << k << " " << (char)k << std::endl; 
        if (loadAllDemos && k >= '1' && k < '1' + MAX_DEMO_TYPES) 
            currentDemoType = (DemoType)((uint)k - '1');

        // Changing mip map level
        if (k == ',') setMipMapLevel((int)currentMipMapLevel + 1);
        if (k == '.') setMipMapLevel((int)currentMipMapLevel - 1);

        // Changing textures
        bool setsNextTexture = k == ';' && voxelTextureGenerator.setNextTexture();
        bool setsPreviousTexture = k == '\'' && voxelTextureGenerator.setPreviousTexture();
        if (setsNextTexture || setsPreviousTexture)
        {
            if (loadAllDemos || currentDemoType == DEBUGDRAW || currentDemoType == DEFERRED_PIPELINE)
                debugDraw->voxelTextureUpdate();
        }
    }
}

void GLFWCALL resize(int w, int h)
{
    glViewport(0, 0, w, h);
    camera.setAspectRatio(w, h);
    windowSize = glm::ivec2(w, h);

    if (loadAllDemos || currentDemoType == DEFERRED_PIPELINE)
        deferredPipeline->resize(w, h);
}

void initGL()
{
    glewExperimental = GL_TRUE;
    glewInit();

    // OpenGL version number
    int* major = new int(); 
    int* minor = new int();
    int* rev = new int();
    glfwGetGLVersion(major, minor, rev);
    if (!((*major == openGLVersion.x && *minor >= openGLVersion.y) || *major > openGLVersion.x))
    {
        std::cout << "Need openGL version " << openGLVersion.x << "." << openGLVersion.y << ". You have " << *major << "." << *minor << "." << std::endl;
    }

    // Debug output
    if(showDebugOutput && Utils::OpenGL::checkExtension("GL_ARB_debug_output"))
    {
        //glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC) glfwGetProcAddress("glDebugMessageControlARB");
        //glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC) glfwGetProcAddress("glDebugMessageCallbackARB");
        //glDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC) glfwGetProcAddress("glDebugMessageInsertARB");
        //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        //glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        //glDebugMessageCallbackARB(&Utils::OpenGL::debugOutput, NULL);
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

bool begin()
{
    initGL();

    camera.setFarNearPlanes(.01f, 100.0f);
    camera.zoom(-2);
    camera.lookAt = glm::vec3(0.5f);

    // set up full screen quad and voxel textures
    fullScreenQuad->begin();
    voxelTexture->begin(voxelGridLength);
    voxelTextureGenerator.begin(voxelTexture);
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
        deferredPipeline->resize(windowSize.x, windowSize.y);
    }

    setMipMapLevel(0);

    return true;
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
    perFrame.uResolution = glm::vec2(windowSize);
    perFrame.uAspect = (float)windowSize.x/windowSize.y;
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
    frameTime += FRAME_TIME_DELTA;
}

void displayFPS(int* frameCount)
{
    *frameCount += 1;
    double currentTime = glfwGetTime();
    if(currentTime >= 1.0)
    {	
        std::ostringstream ss;
		ss << applicationName << " (fps: " << (*frameCount/currentTime) << " )";
        glfwSetWindowTitle(ss.str().c_str());
        glfwSetTime(0.0);
        *frameCount = 0;
    }
}
int main(int argc, char* argv[])
{
    if(!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    if(!glfwOpenWindow(windowSize.x, windowSize.y, 0, 0, 0, 0, 16, 0, GLFW_WINDOW))
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    begin();
    int frameCount = 0;
    bool running = true;
    glfwSetWindowTitle(applicationName.c_str());
    glfwSetWindowSizeCallback(resize);
    glfwSetMousePosCallback(mouseMove);
    glfwSetKeyCallback(key);
    glfwEnable(GLFW_KEY_REPEAT);
    glfwEnable(GLFW_STICKY_KEYS);
    glfwSwapInterval(vsync ? 1 : 0);
    glfwSetTime(0.0);
    
    do
    {
        display();
        glfwSwapBuffers();
        if (showFPS) displayFPS(&frameCount);
        running = !glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED);
    }
    while(running);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
