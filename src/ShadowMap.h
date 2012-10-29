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
    PerFrameUBO* perFrame;
    GLuint perFrameUBO;

    GLuint shadowMapProgram;
    GLuint shadowMapFBO;
    GLuint shadowMapTexture;
    int shadowMapResolution;

    void begin(int shadowMapResolution, CoreEngine* coreEngine, PerFrameUBO* perFrame, GLuint perFrameUBO, ThirdPersonCamera* lightCamera)
    {
        this->shadowMapResolution = shadowMapResolution;
        this->coreEngine = coreEngine;
        this->perFrame = perFrame;
        this->perFrameUBO = perFrameUBO;
        this->lightCamera = lightCamera;

        // Generate the shadow map texture
        glGenTextures(1, &shadowMapTexture);
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, shadowMapResolution, shadowMapResolution);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
        float zeroes[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, zeroes);

        //create shadow map framebuffer
        glGenFramebuffers(1, &shadowMapFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMapFBO);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);   
        glDrawBuffer(GL_NONE);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Activate the shadow map texture
        glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_BINDING);
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
        
        // Create shader
        std::string vertexShaderSource = SHADER_DIRECTORY + "passthrough.vert";
        std::string fragmentShaderSource = SHADER_DIRECTORY + "passthrough.frag";
        shadowMapProgram = Utils::OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    }

    ~ShadowMap()
    {
        glDeleteFramebuffers(1, &shadowMapFBO);
        glDeleteTextures(1, &shadowMapTexture);
    }

    void display()
    {
        // Change viewport to match the size of the shadow map resolution
        Utils::OpenGL::setViewport(shadowMapResolution, shadowMapResolution);
        Utils::OpenGL::setRenderState(true, true, false);
        
        //glm::mat4 worldToLight = lightCamera->createOrthrographicProjectionMatrix() * lightCamera->createViewMatrix();
        glm::mat4 worldToLight = lightCamera->createProjectionMatrix() * lightCamera->createViewMatrix();
        // Offset matrix from NDC space to texture space
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

        // Prepare to render to shadow map FBO
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowMapFBO);
        float clearDepth = 1.0f;
        glClearBufferfv(GL_DEPTH, 0, &clearDepth);

        // Set polygon offset and front-face culling
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPolygonOffset(-1.1f, -4.0f);
        glCullFace(GL_FRONT);

        // Render into shadow map
        glUseProgram(shadowMapProgram);
        coreEngine->display();

        // Disable polygon offset, re-enable back-face culling
        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);

        // Unbind FBO
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

};
