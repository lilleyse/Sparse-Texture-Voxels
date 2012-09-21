#include <glf.hpp>
#include "ShaderConstants.h"
#include "Camera.h"
#include "DebugDraw.h"

namespace
{
    std::string const SAMPLE_NAME("Sparse Texture Voxels");
    const int SAMPLE_SIZE_WIDTH(320);
    const int SAMPLE_SIZE_HEIGHT(240);
    const int SAMPLE_MAJOR_VERSION(3);
    const int SAMPLE_MINOR_VERSION(3);

    glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));

    GLuint fullScreenProgram;
    GLuint fullScreenVertexArray;
    GLuint voxelTexture;
    GLuint perFrameUBO;

    ThirdPersonCamera camera;
    DebugDraw debugDraw;

    unsigned int sideLength = 64;
    unsigned int numMipMapLevels;
    unsigned int debugMipMapLevel;
    float frameTime = 0.0f;
    const float FRAME_TIME_DELTA = 0.01f;
}

unsigned int getNumMipMapLevels(unsigned int size)
{
    return (unsigned int)(glm::log2(float(size)) + 1.5);
}

void initGL()
{

    // Debug output
    if(glf::checkExtension("GL_ARB_debug_output"))
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        glDebugMessageCallbackARB(&glf::debugOutput, NULL);
    }
    else
    {
        printf("debug output extension not found");
    }

    // Create a dense 3D texture
    numMipMapLevels = getNumMipMapLevels(sideLength);
    glGenTextures(1, &voxelTexture);
    glActiveTexture(GL_TEXTURE0 + VOXEL_TEXTURE_3D_BINDING);
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glTexStorage3D(GL_TEXTURE_3D, numMipMapLevels, GL_RGBA8, sideLength, sideLength, sideLength);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, zeroes);

    // Create a thinly voxelized sphere shape
    std::vector<glm::u8vec4> textureData(sideLength*sideLength*sideLength);

    glm::vec3 center = glm::vec3(sideLength/2, sideLength/2, sideLength/2);
    float radius = sideLength/4.0f;

    for(unsigned int i = 0; i < sideLength; i++)
    {
        for(unsigned int j = 0; j < sideLength; j++)
        {
            for(unsigned int k = 0; k < sideLength; k++)
            {
                unsigned int textureIndex = sideLength*sideLength*i + sideLength*j + k;

                float distanceFromCenter = glm::distance(center, glm::vec3(i,j,k));
                //if(glm::abs(distanceFromCenter - radius) < 1.0f)
                    //textureData[textureIndex] = (glm::u8vec4)(glm::linearRand(glm::vec4(0,0,0,255), glm::vec4(255, 255, 255, 255)));
                    textureData[textureIndex] = glm::u8vec4(255,0,0,127);
                //else
                //    textureData[textureIndex] = glm::u8vec4(0,0,0,0);
            }
        }
    }

    // Fill entire texture (first mipmap level)
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, sideLength, sideLength, sideLength, GL_RGBA, GL_UNSIGNED_BYTE, &textureData[0]);

    // Generate mipmaps automatically
    glGenerateMipmap(GL_TEXTURE_3D);


    // Create buffer objects and vao for a full screen quad

    const unsigned int numVertices = 4;
    glm::vec2 vertices[numVertices];
    vertices[0] = glm::vec2(-1.0, -1.0);
    vertices[1] = glm::vec2(1.0, -1.0);
    vertices[2] = glm::vec2(1.0, 1.0);
    vertices[3] = glm::vec2(-1.0, 1.0);

    const unsigned int numElements = 6;
    unsigned short elements[numElements] = {0, 1, 2, 2, 3, 0};

    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*numVertices, vertices, GL_STATIC_DRAW);

    GLuint elementArrayBuffer;
    glGenBuffers(1, &elementArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*numElements, elements, GL_STATIC_DRAW);

    glGenVertexArrays(1, &fullScreenVertexArray);
    glBindVertexArray(fullScreenVertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glEnableVertexAttribArray(POSITION_ATTR);
    glVertexAttribPointer(POSITION_ATTR, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);

    //Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    // Create per frame uniform buffer object

    glGenBuffers(1, &perFrameUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PerFrameUBO), NULL, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, PER_FRAME_UBO_BINDING, perFrameUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    // Create shader program
    GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, "src/simpleShader.vert");
    GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, "src/simpleShader.frag");

    fullScreenProgram = glCreateProgram();
    glAttachShader(fullScreenProgram, vertexShaderObject);
    glAttachShader(fullScreenProgram, fragmentShaderObject);
    glDeleteShader(vertexShaderObject);
    glDeleteShader(fragmentShaderObject);

    glLinkProgram(fullScreenProgram);
    glf::checkProgram(fullScreenProgram);


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
    camera.rotate(Window.RotationCurrent.x, Window.RotationCurrent.y);
    camera.zoom(-Window.TranlationCurrent.y*0.25);
}

void keyboardEvent(unsigned char keyCode)
{
    if(keyCode == 'w')
    {
        camera.zoom(10);
    }
    else if(keyCode == 's')
    {
        camera.zoom(-10);
    }
    else if(keyCode == 'a')
    {
        camera.pan(-10, 0);
    }
    else if(keyCode == 'd')
    {
        camera.pan(10, 0);
    }
    else if(keyCode == 44)
    {
        if(debugMipMapLevel > 0)
        {
            debugMipMapLevel--;
        }
    }
    else if(keyCode == 46)
    {
        if(debugMipMapLevel < numMipMapLevels - 1)
        {
            debugMipMapLevel++;
        }
    }

}

bool begin()
{

    initGL();
    debugMipMapLevel = 0;
    debugDraw.init();
    debugDraw.createCubesFromVoxels(voxelTexture, sideLength, numMipMapLevels);

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
    perFrame.uCamPosition = camera.cameraPos;
    perFrame.uCamUp = camera.upDir;
    perFrame.uResolution = glm::uvec2(Window.Size.x, Window.Size.y);
    perFrame.uTime = frameTime;
    glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), &perFrame);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //debugDraw.display(debugMipMapLevel);

    glUseProgram(fullScreenProgram);
    glBindVertexArray(fullScreenVertexArray);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

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