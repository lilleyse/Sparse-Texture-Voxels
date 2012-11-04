#pragma once
#include "Utils.h"
#include "ShaderConstants.h"
#include "Camera.h"
#include "engine/CoreEngine.h"
#include "Passthrough.h"

struct ShadowMap
{
    CoreEngine* coreEngine;
    ThirdPersonCamera* lightCamera;
    FullScreenQuad* fullScreenQuad;
    PerFrameUBO* perFrame;
    GLuint perFrameUBO;

    GLuint shadowMapProgram;
    GLuint gaussianBlurXProgram;
    GLuint gaussianBlurYProgram;

    GLuint shadowMapGenFBO;
    
    GLuint shadowMapTextures[2];
    GLuint shadowMapBlurFBO[2];

    GLuint shadowMapBlurSampler;
    GLuint shadowMapMainSampler;

    int shadowMapResolution;

    void begin(int shadowMapResolution, CoreEngine* coreEngine, FullScreenQuad* fullScreenQuad, PerFrameUBO* perFrame, GLuint perFrameUBO, ThirdPersonCamera* lightCamera)
    {
        this->shadowMapResolution = shadowMapResolution;
        this->coreEngine = coreEngine;
        this->fullScreenQuad = fullScreenQuad;
        this->perFrame = perFrame;
        this->perFrameUBO = perFrameUBO;
        this->lightCamera = lightCamera;

        //--------------------------------
        // Generate shadow map FBO
        //--------------------------------

        // Create gaussian blur FBOs. Two textures needed for ping-ponging during blurring step.
        glActiveTexture(GL_TEXTURE0 + NON_USED_TEXTURE);
        glGenFramebuffers(2, shadowMapBlurFBO);
        glGenTextures(2, shadowMapTextures);
        for(uint i = 0; i <= 1; i++)
        {
            // Create texture
            glBindTexture(GL_TEXTURE_2D, shadowMapTextures[i]);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, shadowMapResolution, shadowMapResolution);

            // Create FBO
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMapBlurFBO[i]);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTextures[i], 0);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
        }

        // Generate depth renderbuffer (32F depth).
        GLuint depthRenderbuffer;
        glGenRenderbuffers(1, &depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, shadowMapResolution, shadowMapResolution);

        // Create shadow map gen framebuffer
        glGenFramebuffers(1, &shadowMapGenFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMapGenFBO);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTextures[0], 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        // Gaussian blur sampler
        glGenSamplers(1, &shadowMapBlurSampler);
        glBindSampler(NON_USED_TEXTURE, shadowMapBlurSampler);
        glSamplerParameteri(shadowMapBlurSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	    glSamplerParameteri(shadowMapBlurSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	    glSamplerParameteri(shadowMapBlurSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glSamplerParameteri(shadowMapBlurSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        // Main shadow map sampler
        glGenSamplers(1, &shadowMapMainSampler);
        glBindSampler(NON_USED_TEXTURE, shadowMapMainSampler);
        glSamplerParameteri(shadowMapMainSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(shadowMapMainSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(shadowMapMainSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glSamplerParameteri(shadowMapMainSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glSamplerParameterfv(shadowMapMainSampler, GL_TEXTURE_BORDER_COLOR, zeroes);
        
        // Create shadow map gen shader
        std::string vertexShaderSource = SHADER_DIRECTORY + "shadowMap.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "shadowMap.frag";
        shadowMapProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);

        // Create gaussian blur X shader
        vertexShaderSource = SHADER_DIRECTORY + "fullscreenQuad.vert";
        fragmentShaderSource = SHADER_DIRECTORY + "gaussianBlurX.frag";
        gaussianBlurXProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);

        // Create gaussian blur Y shader
        vertexShaderSource = SHADER_DIRECTORY + "fullscreenQuad.vert";
        fragmentShaderSource = SHADER_DIRECTORY + "gaussianBlurY.frag";
        gaussianBlurYProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }

    ~ShadowMap()
    {
        //glDeleteFramebuffers(1, &shadowMapFBO);
        //glDeleteTextures(1, &shadowMapTexture);
    }

    void display()
    {
        // Get light matrices
        glm::mat4 worldToLight = lightCamera->createOrthrographicProjectionMatrix() * lightCamera->createViewMatrix();
        glm::mat4 offsetMatrix(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f);
        glm::mat4 worldToShadowMap = offsetMatrix * worldToLight;
       
        // Set UBO with light matrices
        glBindBuffer(GL_UNIFORM_BUFFER, perFrameUBO);
        perFrame->uViewProjection = worldToLight;
        perFrame->uWorldToShadowMap = worldToShadowMap;
        perFrame->uLightColor = glm::vec3(1.0f,1.0f,1.0f);
        perFrame->uLightDir = -lightCamera->lookDir;
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameUBO), perFrame);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Generate shadow map
        //glEnable(GL_POLYGON_OFFSET_FILL);
        //glPolygonOffset(1.0, 4.0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMapGenFBO);
        Utils::OpenGL::setViewport(shadowMapResolution, shadowMapResolution);
        Utils::OpenGL::clearColorAndDepth();
        Utils::OpenGL::setRenderState(true, true, true);
        glUseProgram(shadowMapProgram);
        coreEngine->display();
        //glDisable(GL_POLYGON_OFFSET_FILL);

        // Do the gaussian blur
        glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_BINDING);
        glBindSampler(SHADOW_MAP_BINDING, shadowMapBlurSampler);
        for(int i = 0; i < 3; i++)
        {
            // Do a y-direction blur on the 0th texture into the 1st
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMapBlurFBO[1]);
            glBindTexture(GL_TEXTURE_2D, shadowMapTextures[0]);
            glUseProgram(gaussianBlurYProgram);
            fullScreenQuad->display();

            // Do an x-direction blur on the 1st texture into the 0th
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMapBlurFBO[0]);
            glBindTexture(GL_TEXTURE_2D, shadowMapTextures[1]);
            glUseProgram(gaussianBlurXProgram);
            fullScreenQuad->display();
        }

        // Set the shadow map to be the active texture
        glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_BINDING);
        glBindTexture(GL_TEXTURE_2D, shadowMapTextures[0]);
        glBindSampler(SHADOW_MAP_BINDING, shadowMapMainSampler);
        
        // Unbind FBO
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
};
