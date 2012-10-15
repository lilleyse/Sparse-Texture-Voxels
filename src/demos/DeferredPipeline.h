#pragma once

#include "../Utils.h"
#include "../ShaderConstants.h"
#include "../FullScreenQuad.h"
#include "../VoxelTexture.h"
#include "../engine/CoreEngine.h"

class DeferredPipeline
{
private:

    std::vector<uint> fboAttachments;

    GLuint deferredFBO;
    GLuint depthTexture;

    GLuint positionsTexture;
    GLuint colorsTexture;
    GLuint normalsTexture;
    GLuint deferredReadProgram;
    GLuint deferredWriteProgram;

    VoxelTexture* voxelTexture;
    FullScreenQuad* fullScreenQuad;
    CoreEngine* coreEngine;

public:
    void begin(VoxelTexture* voxelTexture, FullScreenQuad* fullScreenQuad, CoreEngine* coreEngine)
    {
        this->voxelTexture = voxelTexture;
        this->fullScreenQuad = fullScreenQuad;
        this->coreEngine = coreEngine;

        // Creation positions texture
        glActiveTexture(GL_TEXTURE0 + DEFERRED_POSITIONS_TEXTURE_BINDING);
        glGenTextures(1, &positionsTexture);
        glBindTexture(GL_TEXTURE_2D, positionsTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Create colors texture
        glActiveTexture(GL_TEXTURE0 + DEFERRED_COLORS_TEXTURE_BINDING);
        glGenTextures(1, &colorsTexture);
        glBindTexture(GL_TEXTURE_2D, colorsTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Create normals texture
        glActiveTexture(GL_TEXTURE0 + DEFERRED_NORMALS_TEXTURE_BINDING);
        glGenTextures(1, &normalsTexture);
        glBindTexture(GL_TEXTURE_2D, normalsTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Create depth texture
        glActiveTexture(GL_TEXTURE0 + NON_USED_TEXTURE);
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // If we go over 10 attachments, then change this number
        fboAttachments = std::vector<uint>(FBO_BINDING_POINT_ARRAY_SIZE, GL_NONE);
        fboAttachments[DEFERRED_POSITIONS_FBO_BINDING] = GL_COLOR_ATTACHMENT0 + DEFERRED_POSITIONS_FBO_BINDING;
        fboAttachments[DEFERRED_COLORS_FBO_BINDING] = GL_COLOR_ATTACHMENT0 + DEFERRED_COLORS_FBO_BINDING;
        fboAttachments[DEFERRED_NORMALS_FBO_BINDING] = GL_COLOR_ATTACHMENT0 + DEFERRED_NORMALS_FBO_BINDING;

        // Create deffered framebuffer object
        glGenFramebuffers(1, &deferredFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, deferredFBO);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, fboAttachments[DEFERRED_POSITIONS_FBO_BINDING], GL_TEXTURE_2D, positionsTexture, 0);  
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, fboAttachments[DEFERRED_COLORS_FBO_BINDING], GL_TEXTURE_2D, colorsTexture, 0); 
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, fboAttachments[DEFERRED_NORMALS_FBO_BINDING], GL_TEXTURE_2D, normalsTexture, 0);
        glDrawBuffers(FBO_BINDING_POINT_ARRAY_SIZE, &fboAttachments[0]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        // Create a program that writes the deferred data
        GLuint vertexShaderObjectWrite = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "mainDeferred.vert");
        GLuint fragmentShaderObjectWrite = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "mainDeferred.frag");

        deferredWriteProgram = glCreateProgram();
        glAttachShader(deferredWriteProgram, vertexShaderObjectWrite);
        glAttachShader(deferredWriteProgram, fragmentShaderObjectWrite);
        glDeleteShader(vertexShaderObjectWrite);
        glDeleteShader(fragmentShaderObjectWrite);

        glLinkProgram(deferredWriteProgram);
        Utils::OpenGL::checkProgram(deferredWriteProgram);

        // Create program that reads the deferred data
        GLuint vertexShaderObjectRead = Utils::OpenGL::createShader(GL_VERTEX_SHADER, SHADER_DIRECTORY + "fullscreen.vert");
        GLuint fragmentShaderObjectRead = Utils::OpenGL::createShader(GL_FRAGMENT_SHADER, SHADER_DIRECTORY + "deferredRead.frag");

        deferredReadProgram = glCreateProgram();
        glAttachShader(deferredReadProgram, vertexShaderObjectRead);
        glAttachShader(deferredReadProgram, fragmentShaderObjectRead);
        glDeleteShader(vertexShaderObjectRead);
        glDeleteShader(fragmentShaderObjectRead);

        glLinkProgram(deferredReadProgram);
        Utils::OpenGL::checkProgram(deferredReadProgram);

        glUseProgram(deferredReadProgram);
        GLuint textureResUniform = glGetUniformLocation(deferredReadProgram, "uTextureRes");
        glUniform1f(textureResUniform, (float)voxelTexture->voxelGridLength);
    }

    void resize(int w, int h)
    {
        // Make sure to resize the textures used in the deferred pipeline
        glActiveTexture(GL_TEXTURE0 + NON_USED_TEXTURE);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glActiveTexture(GL_TEXTURE0 + DEFERRED_POSITIONS_TEXTURE_BINDING);
        glBindTexture(GL_TEXTURE_2D, positionsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);

        glActiveTexture(GL_TEXTURE0 + DEFERRED_COLORS_TEXTURE_BINDING);
        glBindTexture(GL_TEXTURE_2D, colorsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glActiveTexture(GL_TEXTURE0 + DEFERRED_NORMALS_TEXTURE_BINDING);
        glBindTexture(GL_TEXTURE_2D, normalsTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    }

    void display()
    {
        //voxelTexture->enableNearestSampling();
        voxelTexture->enableLinearSampling();
        
        // Bind custom FBO then draw into it
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, deferredFBO);
        
        // Clear depth textures
        float clearDepth = 1.0f;
        glClearBufferfv(GL_DEPTH, 0, &clearDepth);
       
        // Clear color attachment textures (positions, colors, normals)
        float clearColor[4] = {0.0f,0.0f,0.0f,0.0f};
        for(uint i = 0; i < FBO_BINDING_POINT_ARRAY_SIZE; i++)
        {
            if(fboAttachments[i] != GL_NONE)
                glClearBufferfv(GL_COLOR, i, clearColor);
        }
        
        // Write the scene to the fbo's
        glUseProgram(deferredWriteProgram);

        // Render to the FBO
        coreEngine->display();

        // Bind the default window framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);        
        
        // Show the results written to the FBO
        glUseProgram(deferredReadProgram);
        fullScreenQuad->display();
    }
};
