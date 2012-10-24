#include "Utils.h"
#include "ShaderConstants.h"
#include "Camera.h"
#include "VoxelTextureGenerator.h"
#include "Voxelizer.h"
#include "VoxelLighting.h"
#include "Passthrough.h"
#include "engine/CoreEngine.h"
#include "demos/VoxelDebug.h"
#include "demos/VoxelRaycaster.h"
#include "demos/VoxelConetracer.h"
#include "demos/MainRenderer.h"
#include "demos/TriangleDebug.h"

namespace
{
    // Window
    std::string applicationName("Sparse Texture Voxels");
    glm::ivec2 windowSize(600, 400);
    glm::ivec2 openGLVersion(4, 2);
    bool enableMousePicking = true;
    bool showDebugOutput = false;
    bool showFPS = true;
    bool vsync = false;
    
    // Texture settings
    std::string sceneFile = SCENE_DIRECTORY + "cornell.xml";
    uint voxelGridLength = 128;
    uint numMipMapLevels = 0; // If 0, then calculate the number based on the grid length
    uint currentMipMapLevel = 0;
    float specularFOV = 5.0;
    float specularAmount = 0.5;
    bool loadTextures = false;
    const std::string voxelTextures[] = {
        VoxelTextureGenerator::CORNELL_BOX,
        VoxelTextureGenerator::SPHERE,
        VoxelTextureGenerator::CUBE,
        DATA_DIRECTORY + "Bucky.raw",
    };

    // Demo settings
    bool loadAllDemos = true;
    enum DemoType {VOXEL_DEBUG, TRIANGLE_DEBUG, VOXELRAYCASTER, VOXELCONETRACER, MAIN_RENDERER, MAX_DEMO_TYPES};
    DemoType currentDemoType = MAIN_RENDERER;
    VoxelDebug* voxelDebug = new VoxelDebug();
    TriangleDebug* triangleDebug = new TriangleDebug();
    VoxelRaycaster* voxelRaycaster = new VoxelRaycaster();
    VoxelConetracer* voxelConetracer = new VoxelConetracer();
    MainRenderer* mainRenderer = new MainRenderer();
    
    // Other
    int frameCount = 0;
    float frameTime = 0.0f;
    const float FRAME_TIME_DELTA = 0.01f;
    glm::ivec2 mouseClickPos;
    glm::ivec2 currentMousePos;
    Object* currentSelectedObject;
    ThirdPersonCamera* viewCamera = new ThirdPersonCamera();
    ThirdPersonCamera* lightCamera = new ThirdPersonCamera();
    ThirdPersonCamera* currentCamera = viewCamera;
    VoxelTextureGenerator* voxelTextureGenerator = new VoxelTextureGenerator();
    VoxelTexture* voxelTexture = new VoxelTexture();
    Voxelizer* voxelizer = new Voxelizer();
    VoxelLighting* voxelLighting = new VoxelLighting();
    MipMapGenerator* mipMapGenerator = new MipMapGenerator();
    Utils::OpenGL::OpenGLTimer* timer = new Utils::OpenGL::OpenGLTimer();
    CoreEngine* coreEngine = new CoreEngine();
    FullScreenQuad* fullScreenQuad = new FullScreenQuad();
    Passthrough* passthrough = new Passthrough();
    PerFrameUBO* perFrame = new PerFrameUBO();
    GLuint perFrameUBO;  
}

void setMipMapLevel(int level)
{
    currentMipMapLevel = std::max(std::min(level, (int)voxelTexture->numMipMapLevels - 1), 0);    
    if (loadAllDemos || currentDemoType == VOXEL_DEBUG)
        voxelDebug->setMipMapLevel(currentMipMapLevel);
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
        float cameraDistanceFromCenter = glm::length(currentCamera->position);
        if (leftPress)
        {
            float rotateAmount = cameraDistanceFromCenter / 200.0f;
            currentCamera->rotate(-mouseDelta.x * rotateAmount, -mouseDelta.y * rotateAmount);
        }
        else if (rightPress)
        {
            float zoomAmount = cameraDistanceFromCenter / 200.0f;
            currentCamera->zoom(mouseDelta.y * zoomAmount);
        }
        else if (middlePress)
        {
            float panAmount = cameraDistanceFromCenter / 500.0f;
            currentCamera->pan(-mouseDelta.x * panAmount, mouseDelta.y * panAmount);
        }
    }
}

void GLFWCALL mouseClick(int button, int action)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mouseClickPos = currentMousePos;
        }
        else if (action == GLFW_RELEASE)
        {
            if (currentDemoType == MAIN_RENDERER && enableMousePicking && mouseClickPos == currentMousePos)
            {
                currentSelectedObject = 0;
                Utils::Math::Ray ray = Utils::Math::getPickingRay(currentMousePos.x, currentMousePos.y, windowSize.x, windowSize.y, currentCamera->nearPlane, currentCamera->farPlane,  currentCamera->viewMatrix, currentCamera->projectionMatrix);    
                std::vector<Object*> objects = coreEngine->scene->objects;
                float closestIntersection = FLT_MAX;
                for (uint i = 0; i < objects.size(); i++)
                {
                    Object* object = objects.at(i);
                    glm::mat4 transformationMatrix = object->position.modelMatrix;
                    glm::mat4 invTransformationMatrix = glm::inverse(transformationMatrix);
                    Utils::Math::Ray transformedRay = ray.transform(invTransformationMatrix);
                    glm::vec3 boundingBoxMin = -object->mesh->extents;
                    glm::vec3 boundingBoxMax = object->mesh->extents;
                    float boundingBoxIntersection = Utils::Math::rayBoundingBoxIntersect(transformedRay, boundingBoxMin, boundingBoxMax); 
                    if (boundingBoxIntersection > 0.0f && boundingBoxIntersection < closestIntersection)
                    {
                        currentSelectedObject = object;
                        closestIntersection = boundingBoxIntersection;
                    }
                }
            }
        }
    }
}

void GLFWCALL keyPress(int k, int action)
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

        // Enable linear sampling
        if (k == 'L') voxelTexture->changeSamplerType();

        //Switch between light and regular camera
        if (k == GLFW_KEY_SPACE)
        {
            currentCamera = (currentCamera == viewCamera) ? lightCamera : viewCamera;
        }

        // Changing textures
        if (loadTextures)
        {
            bool setsNextTexture = k == ';' && voxelTextureGenerator->setNextTexture();
            bool setsPreviousTexture = k == '\'' && voxelTextureGenerator->setPreviousTexture();
            if (setsNextTexture || setsPreviousTexture)
                if (loadAllDemos || currentDemoType == VOXEL_DEBUG)
                    voxelDebug->voxelTextureUpdate();
        }
    }
}

void processKeyDown()
{
    // This method checks if keys are down every frame
    bool shiftDown = glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS || glfwGetKey(GLFW_KEY_RSHIFT) == GLFW_PRESS;
    if (currentDemoType == MAIN_RENDERER)
    {    
        // Transforming selected objects
        if (enableMousePicking && currentSelectedObject != 0)
        {
            float translationAmount = 0.01f;
            float rotationAmount = 0.5f;
            float scaleAmount = 0.01f;
            if(glfwGetKey('W') == GLFW_PRESS || glfwGetKey(GLFW_KEY_UP) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(0.0f, translationAmount, 0.0f));
            if(glfwGetKey('S') == GLFW_PRESS || glfwGetKey(GLFW_KEY_DOWN) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(0.0f, -translationAmount, 0.0f));
            if(glfwGetKey('A') == GLFW_PRESS || glfwGetKey(GLFW_KEY_LEFT) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(-translationAmount, 0.0f, 0.0f));
            if(glfwGetKey('D') == GLFW_PRESS || glfwGetKey(GLFW_KEY_RIGHT) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(translationAmount, 0.0f, 0.0f));
            if(glfwGetKey('Q') == GLFW_PRESS || glfwGetKey(GLFW_KEY_END) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(0.0f, 0.0f, translationAmount));
            if(glfwGetKey('E') == GLFW_PRESS || glfwGetKey(GLFW_KEY_HOME) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(0.0f, 0.0f, -translationAmount));
            if(shiftDown && glfwGetKey('R') == GLFW_PRESS) currentSelectedObject->rotate(glm::vec3(0.0f, 1.0f, 0.0f), rotationAmount);
            else if(glfwGetKey('R') == GLFW_PRESS) currentSelectedObject->rotate(glm::vec3(0.0f, 1.0f, 0.0f), -rotationAmount);
            if(shiftDown && glfwGetKey('T') == GLFW_PRESS) currentSelectedObject->scale(1.0f - scaleAmount);
            else if(glfwGetKey('T') == GLFW_PRESS) currentSelectedObject->scale(1.0f + scaleAmount);
        }

        // Changing specular values
        float specularFOVChange = 0.2f;
        float specularAmountChange = 0.004f;
        if (glfwGetKey('F') == GLFW_PRESS) specularFOV = glm::clamp(specularFOV + specularFOVChange * (shiftDown ? -1.0f : 1.0f), 0.01f, 50.0f);
        if (glfwGetKey('G') == GLFW_PRESS) specularAmount = glm::clamp(specularAmount + specularAmountChange * (shiftDown ? -1.0f : 1.0f), 0.0f, 1.0f);
    }
}

void GLFWCALL resize(int w, int h)
{
    glViewport(0, 0, w, h);
    viewCamera->setAspectRatio(w, h);
    lightCamera->setAspectRatio(w, h);
    windowSize = glm::ivec2(w, h);
}

void clearBuffers()
{
    float clearColor[4] = {0.0f,0.0f,0.0f,0.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor);
    float clearDepth = 1.0f;
    glClearBufferfv(GL_DEPTH, 0, &clearDepth);
}

void setFBO()
{
    // Update the per frame UBO
    perFrame->uViewProjection = currentCamera->createViewProjectionMatrix();    
    perFrame->uCamLookAt = currentCamera->lookAt;
    perFrame->uCamPos = currentCamera->position;
    perFrame->uCamUp = currentCamera->upDir;
    perFrame->uResolution = glm::vec2(windowSize);
    perFrame->uAspect = (float)windowSize.x/windowSize.y;
    perFrame->uTime = frameTime;
    perFrame->uFOV = currentCamera->fieldOfView;
    perFrame->uTextureRes = (float)voxelTexture->voxelGridLength;
    perFrame->uNumMips = (float)voxelTexture->numMipMapLevels;
    perFrame->uSpecularFOV = specularFOV;
    perFrame->uSpecularAmount = specularAmount;
    perFrame->uLightColor = glm::vec3(1.0f,1.0f,1.0f);
    perFrame->uLightDir = glm::vec3(0.0f, 0.0f, 1.0f);
    //perFrame->timestamp handled by voxelizer

    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
    else printf("debug output extension not found or disabled\n");
    
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

    // camera
    viewCamera->setFarNearPlanes(.01f, 100.0f);
    viewCamera->zoom(3.0f);
    viewCamera->lookAt = glm::vec3(0.5f);
    lightCamera->setFarNearPlanes(.01f, 100.0f);
    lightCamera->zoom(3.0f);
    lightCamera->lookAt = glm::vec3(0.5f);

    // set up miscellaneous things
    timer->begin();
    coreEngine->begin(sceneFile);
    fullScreenQuad->begin();
    passthrough->begin(coreEngine);
    voxelTexture->begin(voxelGridLength, numMipMapLevels);
    mipMapGenerator->begin(voxelTexture, fullScreenQuad);
    voxelTextureGenerator->begin(voxelTexture, mipMapGenerator);
    voxelizer->begin(voxelTexture, coreEngine, perFrame, perFrameUBO);
    voxelLighting->begin(voxelTexture, coreEngine, passthrough, perFrame, perFrameUBO);

    // blank slate
    clearBuffers();
    setFBO();

    // voxelize and light the scene
    voxelizer->voxelizeScene();
    voxelLighting->lightScene(lightCamera->createViewProjectionMatrix());
    
    // mipmap
    if (loadTextures)
    {
        // create procedural textures
        uint numInitialTextures = sizeof(voxelTextures) / sizeof(voxelTextures[0]);
        for (uint i = 0; i < numInitialTextures; i++)
            voxelTextureGenerator->createTexture(voxelTextures[i]);

        // Load scene texture onto cpu
        voxelTextureGenerator->createTextureFromVoxelTexture(sceneFile);
        voxelTextureGenerator->setTexture(sceneFile);
    }
    else mipMapGenerator->generateMipMapGPU();

    // init demos
    if (loadAllDemos || currentDemoType == VOXEL_DEBUG) 
        voxelDebug->begin(voxelTexture);
    if (loadAllDemos || currentDemoType == TRIANGLE_DEBUG)
        triangleDebug->begin(coreEngine);
    if (loadAllDemos || currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->begin(voxelTexture, fullScreenQuad);
    if (loadAllDemos || currentDemoType == VOXELCONETRACER)
        voxelConetracer->begin(voxelTexture, fullScreenQuad);
    if (loadAllDemos || currentDemoType == MAIN_RENDERER)
        mainRenderer->begin(coreEngine, passthrough);

    setMipMapLevel(currentMipMapLevel);
}

void display()
{
    // blank slate
    clearBuffers();
    setFBO();
    
    // Update the scene
    if (currentDemoType == MAIN_RENDERER)
    {
        coreEngine->updateScene();
        voxelizer->voxelizeScene();
        voxelLighting->lightScene(lightCamera->createViewProjectionMatrix());
        mipMapGenerator->generateMipMapGPU();
    }

    // blank slate
    clearBuffers();
    setFBO();

    // Display demo
    if (currentDemoType == VOXEL_DEBUG)
        voxelDebug->display();
    else if (currentDemoType == TRIANGLE_DEBUG)
        triangleDebug->display();
    else if (currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->display(); 
    else if (currentDemoType == VOXELCONETRACER)  
        voxelConetracer->display();
    else if (currentDemoType == MAIN_RENDERER)
        mainRenderer->display();
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
    glfwSetMouseButtonCallback(mouseClick);
    glfwSetKeyCallback(keyPress);
    glfwEnable(GLFW_AUTO_POLL_EVENTS);
    glfwSwapInterval(vsync ? 1 : 0);
    glfwSetTime(0.0);

    bool running = true;
    do
    {
        processKeyDown();
        display();
        frameTime += FRAME_TIME_DELTA;
        if (showFPS) displayFPS();
        glfwSwapBuffers();
        running = !glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED);
    }
    while(running);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
