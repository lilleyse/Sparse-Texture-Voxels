#pragma once

#include "Utils.h"

struct Camera
{
    float currXZRads;
    float currYRads;
    glm::vec3 upDir;
    glm::vec3 lookDir;
    glm::vec3 lookAt;
    glm::vec3 position;

    float nearPlane;
    float farPlane;
    float fieldOfView; //degrees
    float aspectRatio;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    Camera()
    {
        currXZRads = 0.0f;
        currYRads = 0.0f;
        nearPlane = 1.0f;
        farPlane = 1000.0f;
        fieldOfView = 30.0f;
        aspectRatio = 1.0f;
        position = glm::vec3(0,0,0);
        lookDir = glm::vec3(0,0,1);
    }

    virtual ~Camera(){}

    void setFarNearPlanes(float nearPlane, float farPlane)
    {
        this->nearPlane = nearPlane;
        this->farPlane = farPlane;
    }
    void setAspectRatio(int width, int height)
    {
        aspectRatio = (float)width/height;
    }

    glm::mat4 createOrthrographicProjectionMatrix()
    {
        float side = 2.0f/2.0f;//-sqrt(3.0f)/2.0f; // diagonal of cube (sqrt(3)) is the greatest length of the scene that the light can witness
        projectionMatrix = glm::ortho(-side, side, -side, side, -10000.0f, 1000.0f);
        return projectionMatrix;
    }
    glm::mat4 createPerspectiveProjectionMatrix()
    {
        projectionMatrix = glm::perspective(45.0f, aspectRatio, nearPlane, farPlane);
        return projectionMatrix;
    }

    virtual glm::mat4 createViewMatrix() = 0;
    virtual glm::vec3 getPosition() = 0;

    virtual void rotate(float x, float y) = 0;
    virtual void zoom(float delta) = 0;
    virtual void pan(float x, float y) = 0;
};

struct ThirdPersonCamera : public Camera
{
    float radius;

    ThirdPersonCamera() : Camera()
    {
        radius = 5.0f;
        createViewMatrix();
    }
    ~ThirdPersonCamera(){}

    void pan(float x, float y)
    {
        glm::vec3 right = glm::normalize(glm::cross(this->lookDir,this->upDir));
        glm::vec3 up = this->upDir;
        glm::vec3 moveX = x*right;
        glm::vec3 moveY = y*up;
        this->lookAt += moveX;
        this->lookAt += moveY;
    }
    void rotate(float x, float y)
    {
        this->currXZRads += x;
        this->currYRads += y;
    }

    void zoom(float distance)
    {
        this->radius -= distance;
    }

    glm::mat4 createViewMatrix(void)
    {
        float cosa = cosf(currXZRads);
        float sina = sinf(currXZRads);

        glm::vec3 currPos(sina, 0.0f, cosa);
        glm::vec3 UpRotAxis(currPos.z, currPos.y, -currPos.x);

        glm::mat4 xRotation = glm::rotate(glm::mat4(1.0f), glm::degrees(currYRads), UpRotAxis);
        currPos = glm::vec3(xRotation * glm::vec4(currPos, 0.0));

        glm::vec3 tempVec = currPos * float(radius);
        this->position = tempVec + lookAt;

        this->upDir = glm::normalize(glm::cross(currPos, UpRotAxis));
        this->lookDir = glm::normalize(this->lookAt - this->position);

        viewMatrix = glm::lookAt(position, position + lookDir, upDir);
        return viewMatrix;
    }

    glm::vec3 getPosition()
    {
        float cosa = cosf(currXZRads);
        float sina = sinf(currXZRads);

        glm::vec3 currPos(sina, 0.0f, cosa);
        glm::vec3 UpRotAxis(currPos.z, currPos.y, -currPos.x);

        glm::mat4 xRotation = glm::rotate(glm::mat4(1.0f), glm::degrees(currYRads), UpRotAxis);
        currPos = glm::vec3(xRotation * glm::vec4(currPos, 0.0));

        glm::vec3 tempVec = currPos * float(radius);
        glm::vec3 pos = tempVec + lookAt;
        return pos;
    }
};

struct FirstPersonCamera : public Camera
{
    FirstPersonCamera() : Camera()
    {
        createViewMatrix();
    }
    ~FirstPersonCamera(){}

    void pan(float x, float y)
    {
        glm::vec3 right = glm::normalize(glm::cross(this->lookDir,this->upDir));
        glm::vec3 up = this->upDir;
        glm::vec3 moveX = x*right;
        glm::vec3 moveY = y*up;
        this->position += moveX;
        this->position += moveY;
    }

    void rotate(float x, float y)
    {
        this->currXZRads += x;
        this->currYRads -= y;
    }

    void zoom(float distance)
    {
        this->position += distance*this->lookDir;
    }

    glm::mat4 createViewMatrix(void)
    {
        float cosa = cosf(currXZRads);
        float sina = sinf(currXZRads);
        glm::vec3 currPos(sina, 0.0f, cosa);

        glm::vec3 UpRotAxis;
        UpRotAxis.x = currPos.z;
        UpRotAxis.y = currPos.y;
        UpRotAxis.z = -currPos.x;

        glm::mat4 xRotation = glm::rotate(glm::mat4(1.0f), glm::degrees(currYRads), UpRotAxis);
        currPos = glm::vec3(xRotation * glm::vec4(currPos, 0.0));

        this->lookDir = glm::normalize(currPos);
        this->upDir = glm::normalize(glm::cross(currPos, UpRotAxis));
        this->lookAt = position + lookDir;

        viewMatrix = glm::lookAt(position, position + lookDir, upDir);
        return viewMatrix;
    }

    glm::vec3 getPosition()
    {
        return this->position;
    }
};