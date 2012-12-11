#include "Utils.h"
#include "ShaderConstants.h"
#include "Camera.h"
#include "Voxelizer.h"
#include "Passthrough.h"
#include "MipMapGenerator.h"
#include "VoxelClean.h"
#include "ShadowMap.h"
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
    glm::ivec2 windowSize(800, 533);
    glm::ivec2 openGLVersion(4, 2);
    bool enableMousePicking = true;
    bool showDebugOutput = true;
    bool showFPS = true;
    bool vsync = false;
    
    // Texture settings
    std::string sceneFile = SCENE_DIRECTORY + "sponza.xml";
    uint voxelGridLength = 128;
    float voxelRegionWorldSize = 50.0f;
    uint numVoxelCascades = 3;
    uint shadowMapResolution = 1024;
    uint numMipMapLevels = 6; // If 0, then calculate the number based on the grid length
    uint currentMipMapLevel = 0;
    uint currentCascade = 0;
    float specularFOV = 0.01f;
    float specularAmount = 0.8f;

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
    FirstPersonCamera* viewCamera = new FirstPersonCamera();
    ThirdPersonCamera* lightCamera = new ThirdPersonCamera();
    FirstPersonCamera* observerCamera = new FirstPersonCamera();
    Camera* currentCamera = viewCamera;
    VoxelTexture* voxelTexture = new VoxelTexture();
    Voxelizer* voxelizer = new Voxelizer();
    VoxelClean* voxelClean = new VoxelClean();
    MipMapGenerator* mipMapGenerator = new MipMapGenerator();
    Utils::OpenGL::OpenGLTimer* timer = new Utils::OpenGL::OpenGLTimer();
    CoreEngine* coreEngine = new CoreEngine();
    FullScreenQuad* fullScreenQuad = new FullScreenQuad();
    Passthrough* passthrough = new Passthrough();
    ShadowMap* shadowMap = new ShadowMap();
    PerFrameUBO* perFrame = new PerFrameUBO();
    GLuint perFrameUBO;
}

void GLFWCALL mouseMove(int x, int y)
{
    glm::ivec2 newMousePos = glm::ivec2(x,y);
    glm::ivec2 mouseDelta = newMousePos - currentMousePos; 
    currentMousePos = newMousePos;

    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        float rotateAmount = 0.005f;
        currentCamera->rotate(-mouseDelta.x * rotateAmount, -mouseDelta.y * rotateAmount);
    }
    else if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        float zoomAmount = 0.002f*coreEngine->scene->radius;
        currentCamera->zoom(mouseDelta.y * zoomAmount);
    }
    else if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
    {
        float panAmount = 0.002f*coreEngine->scene->radius;
        currentCamera->pan(-mouseDelta.x * panAmount, mouseDelta.y * panAmount);
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
            if (enableMousePicking && mouseClickPos == currentMousePos)
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
    bool shiftDown = glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS || glfwGetKey(GLFW_KEY_RSHIFT) == GLFW_PRESS;
    if (action == GLFW_RELEASE)
    {
        // Changing demo (number keys and numpad)
        if (loadAllDemos && k >= '1' && k < '1' + MAX_DEMO_TYPES)
        {
            currentDemoType = (DemoType)((uint)k - '1');
            if(currentDemoType == VOXEL_DEBUG) voxelDebug->voxelTextureUpdate();
        }
        if (loadAllDemos && k >= GLFW_KEY_KP_1 && k < GLFW_KEY_KP_1 + MAX_DEMO_TYPES)
        {
            currentDemoType = (DemoType)((uint)k - GLFW_KEY_KP_1);
            if(currentDemoType == VOXEL_DEBUG) voxelDebug->voxelTextureUpdate();
        }            

        // Changing mip map level
        int newMipLevel = currentMipMapLevel + int(k == '.') - int(k == ',');
        newMipLevel = glm::clamp(newMipLevel, 0, int(voxelTexture->numMipMapLevels - 1));
        currentMipMapLevel = newMipLevel;
        
        //int newCascade = currentCascade + int(shiftDown)*(int(k == '.') - int(k == ','));
        //newCascade = glm::clamp(newCascade, 0, int(voxelTexture->numCascades - 1));
        //currentCascade = newCascade;

        // Enable linear sampling
        if (k == 'L') voxelTexture->changeSamplerType();

        //Switch between light and regular camera
        if (k == GLFW_KEY_SPACE)
        {
            if(currentCamera == viewCamera) currentCamera = lightCamera;
            else if(currentCamera == lightCamera) currentCamera = observerCamera;
            else if(currentCamera == observerCamera) currentCamera = viewCamera;
        }
    }
}

void processKeyDown()
{
    // This method checks if keys are down every frame
    bool shiftDown = glfwGetKey(GLFW_KEY_LSHIFT) == GLFW_PRESS || glfwGetKey(GLFW_KEY_RSHIFT) == GLFW_PRESS;
  
    // Transforming selected objects
    if (enableMousePicking && currentSelectedObject != 0)
    {
        float translationAmount = 0.01f;
        float rotationAmount = 0.5f;
        float scaleAmount = 0.01f;
        if(glfwGetKey(GLFW_KEY_UP) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(0.0f, translationAmount, 0.0f));
        if(glfwGetKey(GLFW_KEY_DOWN) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(0.0f, -translationAmount, 0.0f));
        if(glfwGetKey(GLFW_KEY_LEFT) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(-translationAmount, 0.0f, 0.0f));
        if(glfwGetKey(GLFW_KEY_RIGHT) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(translationAmount, 0.0f, 0.0f));
        if(glfwGetKey(GLFW_KEY_END) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(0.0f, 0.0f, translationAmount));
        if(glfwGetKey(GLFW_KEY_HOME) == GLFW_PRESS) currentSelectedObject->translate(glm::vec3(0.0f, 0.0f, -translationAmount));
        if(shiftDown && glfwGetKey('R') == GLFW_PRESS) currentSelectedObject->rotate(glm::vec3(0.0f, 1.0f, 0.0f), rotationAmount);
        else if(glfwGetKey('R') == GLFW_PRESS) currentSelectedObject->rotate(glm::vec3(0.0f, 1.0f, 0.0f), -rotationAmount);
        if(shiftDown && glfwGetKey('T') == GLFW_PRESS) currentSelectedObject->scale(1.0f - scaleAmount);
        else if(glfwGetKey('T') == GLFW_PRESS) currentSelectedObject->scale(1.0f + scaleAmount);
    }

    float zoomSpeed = 0.001f*coreEngine->scene->radius;
    float panSpeed = 0.0007f*coreEngine->scene->radius;
    if(glfwGetKey('W') == GLFW_PRESS) currentCamera->zoom(zoomSpeed);
    if(glfwGetKey('S') == GLFW_PRESS) currentCamera->zoom(-zoomSpeed);
    if(glfwGetKey('A') == GLFW_PRESS) currentCamera->pan(-panSpeed,0.0f);
    if(glfwGetKey('D') == GLFW_PRESS) currentCamera->pan(panSpeed,0.0f);

    // Changing specular values
    float specularFOVChange = 0.2f;
    float specularAmountChange = 0.004f;
    if (glfwGetKey('F') == GLFW_PRESS) specularFOV = glm::clamp(specularFOV + specularFOVChange * (shiftDown ? -1.0f : 1.0f), 0.01f, 50.0f);
    if (glfwGetKey('G') == GLFW_PRESS) specularAmount = glm::clamp(specularAmount + specularAmountChange * (shiftDown ? -1.0f : 1.0f), 0.0f, 1.0f);
}

void GLFWCALL resize(int w, int h)
{
    Utils::OpenGL::setScreenSize(w, h);
    viewCamera->setAspectRatio(w, h);
    observerCamera->setAspectRatio(w, h);
    windowSize = glm::ivec2(w, h);
}

void setUBO()
{
    // Update the per frame UBO
    perFrame->uViewProjection = currentCamera->createPerspectiveProjectionMatrix() * currentCamera->createViewMatrix();    
    perFrame->uCamLookAt = currentCamera->lookAt;
    perFrame->uCamPos = currentCamera->position;
    perFrame->uCamUp = currentCamera->upDir;
    perFrame->uScreenRes = glm::vec2(windowSize);
    perFrame->uAspect = (float)windowSize.x/windowSize.y;
    perFrame->uTime = frameTime;
    perFrame->uFOV = currentCamera->fieldOfView;
    perFrame->uVoxelRes = (float)voxelTexture->voxelGridLength;
    perFrame->uNumMips = (float)voxelTexture->numMipMapLevels;
    perFrame->uSpecularFOV = specularFOV;
    perFrame->uSpecularAmount = specularAmount;
    perFrame->uCurrentMipLevel = currentMipMapLevel;
    perFrame->uCurrentCascade = currentCascade;

    perFrame->uVoxelRegionWorld = glm::vec4(viewCamera->position, voxelRegionWorldSize);
    //float myvoxelSize = 16.0f*voxelRegionWorldSize/perFrame->uVoxelRes;
    //perFrame->uVoxelRegionWorld = glm::vec4(viewCamera->position - glm::vec3(voxelRegionWorldSize/2.0f), voxelRegionWorldSize);
    //perFrame->uVoxelRegionWorld = glm::vec4( glm::vec3( glm::floor(perFrame->uVoxelRegionWorld/myvoxelSize)*myvoxelSize ), perFrame->uVoxelRegionWorld.w);

    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void updateLightObject()
{
    coreEngine->scene->lightObject->setTranslation(lightCamera->getPosition());
}

void initGL()
{
    gl3wInit();

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
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    // Enable depth test
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
}

void initCameras()
{
    viewCamera->setFarNearPlanes(.01f, 1000.0f);
    viewCamera->position = glm::vec3(0.5f,0.35f,0.5f);
    viewCamera->rotate(1.57f,0.0f);

    observerCamera->setFarNearPlanes(.01f, 1000.0f);
    observerCamera->position = glm::vec3(0.5f,0.35f,0.5f);
    observerCamera->rotate(1.57f,0.0f);

    lightCamera->setAspectRatio(shadowMapResolution, shadowMapResolution);
    lightCamera->setFarNearPlanes(.01f, 1000.0f);
    lightCamera->zoom(4.0f);
    lightCamera->lookAt = glm::vec3(0.5f);
    lightCamera->rotate(-1.52f,-0.27f);
    lightCamera->zoom(0.8f);
    
}

void begin()
{
    initGL();

    coreEngine->begin(sceneFile);
    initCameras();

    // set up miscellaneous things
    timer->begin();
    fullScreenQuad->begin();
    passthrough->begin(coreEngine);
    voxelTexture->begin(voxelGridLength, numMipMapLevels, numVoxelCascades);        
    voxelClean->begin(voxelTexture, fullScreenQuad);
    voxelizer->begin(voxelTexture, coreEngine, perFrame, perFrameUBO);
    mipMapGenerator->begin(voxelTexture, fullScreenQuad, perFrame, perFrameUBO);
    shadowMap->begin(shadowMapResolution, coreEngine, fullScreenQuad, lightCamera, perFrame, perFrameUBO);

    // init demos
    if (loadAllDemos || currentDemoType == VOXEL_DEBUG) 
        voxelDebug->begin(voxelTexture, perFrame);
    if (loadAllDemos || currentDemoType == TRIANGLE_DEBUG)
        triangleDebug->begin(coreEngine);
    if (loadAllDemos || currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->begin(fullScreenQuad);
    if (loadAllDemos || currentDemoType == VOXELCONETRACER)
        voxelConetracer->begin(fullScreenQuad);
    if (loadAllDemos || currentDemoType == MAIN_RENDERER)
        mainRenderer->begin(coreEngine, passthrough);
}

void display()
{
    // blank slate
    Utils::OpenGL::clearColorAndDepth();
    setUBO();
    updateLightObject();
    coreEngine->updateScene();

    // Display demo
    if (currentDemoType == VOXEL_DEBUG)
        voxelDebug->display();
    else if (currentDemoType == TRIANGLE_DEBUG)
    {
        shadowMap->display();
        //voxelClean->clean();
        //voxelizer->voxelizeScene();
        //setUBO();
        triangleDebug->display();
        //voxelDebug->voxelTextureUpdate();
        //voxelDebug->display();
    }
    else if (currentDemoType == VOXELRAYCASTER)
        voxelRaycaster->display(); 
    else if (currentDemoType == VOXELCONETRACER)  
        voxelConetracer->display();
    else if (currentDemoType == MAIN_RENDERER) {
        // Update the scene
        shadowMap->display();
        voxelClean->clean();
        voxelizer->voxelizeScene();
        mipMapGenerator->generateMipMap();
        setUBO();
        mainRenderer->display(); 
    }
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
