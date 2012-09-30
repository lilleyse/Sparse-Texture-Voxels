#pragma once
#include <glf.hpp>
#include "../Utils.h"
#include "../ShaderConstants.h"
#include "../VoxelTexture.h"
#include "../FullScreenQuad.h"

class DeferredPipeline
{
private:

    GLuint deferredFBO;
    GLuint depthTexture;

    static const int numColorAttachments = 3;
    GLuint positionsTexture;
    GLuint colorsTexture;
    GLuint normalsTexture;


    GLuint deferredWriteProgram;
    GLuint deferredReadProgram;

public:
    void begin(VoxelTexture* voxelTexture, int screenWidth, int screenHeight)
    {
        // Creation positions texture
        glGenTextures(1, &positionsTexture);
        glBindTexture(GL_TEXTURE_2D, positionsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Create colors texture
        glGenTextures(1, &colorsTexture);
        glBindTexture(GL_TEXTURE_2D, colorsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Create normals texture
        glGenTextures(1, &normalsTexture);
        glBindTexture(GL_TEXTURE_2D, normalsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Create depth texture
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


        // Create deffered framebuffer object
        glGenFramebuffers(1, &deferredFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, deferredFBO);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionsTexture, 0);  
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, colorsTexture, 0); 
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, normalsTexture, 0);
        uint fboAttachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, fboAttachments);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Bind all the textures so we can read back from them later in a separate shader
        glActiveTexture(GL_TEXTURE0 + DEFERRED_POSITIONS_BINDING);
        glBindTexture(GL_TEXTURE_2D, positionsTexture);

        glActiveTexture(GL_TEXTURE0 + DEFERRED_COLORS_BINDING);
        glBindTexture(GL_TEXTURE_2D, colorsTexture);

        glActiveTexture(GL_TEXTURE0 + DEFERRED_NORMALS_BINDING);
        glBindTexture(GL_TEXTURE_2D, normalsTexture);

        // Create program that writes the deferred data
        GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, "src/shaders/voxelDebug.vert");
        GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, "src/shaders/deferredWrite.frag");

        deferredWriteProgram = glCreateProgram();
        glAttachShader(deferredWriteProgram, vertexShaderObject);
        glAttachShader(deferredWriteProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(deferredWriteProgram);
        glf::checkProgram(deferredWriteProgram);

        // Create program that reads the deferred data
        vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, "src/shaders/fullscreen.vert");
        fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, "src/shaders/deferredRead.frag");

        deferredReadProgram = glCreateProgram();
        glAttachShader(deferredReadProgram, vertexShaderObject);
        glAttachShader(deferredReadProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(deferredReadProgram);
        glf::checkProgram(deferredReadProgram);

    }

    void resize(int w, int h)
    {
        // Make sure to resize the textures used in the deferred pipeline
        glActiveTexture(GL_TEXTURE0 + DEFERRED_POSITIONS_BINDING);
        glBindTexture(GL_TEXTURE_2D, positionsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);

        glActiveTexture(GL_TEXTURE0 + DEFERRED_COLORS_BINDING);
        glBindTexture(GL_TEXTURE_2D, colorsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glActiveTexture(GL_TEXTURE0 + DEFERRED_NORMALS_BINDING);
        glBindTexture(GL_TEXTURE_2D, normalsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    }

    void display(FullScreenQuad& fullScreenQuad, DebugDraw& debugDraw)
    {

        // Bind custom FBO then draw into it
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, deferredFBO);
        
        // Clear depth textures
        float clearDepth = 1.0f;
        glClearBufferfv(GL_DEPTH, 0, &clearDepth);
       
        // Clear color attachment textures (positions, colors, normals)
        float clearColor[4] = {0.0f,0.0f,0.0f,0.0f};
        for(int i = 0; i < numColorAttachments; i++)
        {
            glClearBufferfv(GL_COLOR, i, clearColor);
        }
        
        // Write to the FBO
        debugDraw.display(deferredWriteProgram);

        // Bind the default window framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);        
        
        // Show the results written to the FBO
        glUseProgram(deferredReadProgram);
        fullScreenQuad.display();
    }
};
