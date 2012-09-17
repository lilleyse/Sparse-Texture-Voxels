#pragma once
#include <glf.hpp>

struct Camera
{
	float currXZRads;
	float currYRads;
	glm::vec3 upDir;
	glm::vec3 lookDir;
	glm::vec3 cameraPos;

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
		fieldOfView = 45.0f;
		aspectRatio = 1.0f;
		lookDir = glm::vec3(0,0,1);
	}

	virtual ~Camera(){};

	void setFarNearPlanes(float nearPlane, float farPlane)
	{
		this->nearPlane = nearPlane;
		this->farPlane = farPlane;
	}
	void setAspectRatio(int width, int height)
	{
		aspectRatio = (float)Window.Size.x / Window.Size.y;
	}

	glm::mat4 createProjectionMatrix()
	{
		projectionMatrix = glm::perspective(45.0f, aspectRatio, nearPlane, farPlane);
		return projectionMatrix;
	}

	virtual glm::mat4 createViewMatrix() = 0;

	virtual void rotate(float x, float y) = 0;
	virtual void zoom(float delta) = 0;
	virtual void pan(float x, float y) = 0;
};

struct ThirdPersonCamera : public Camera
{
	float radius;
	glm::vec3 lookAt;

	ThirdPersonCamera() : Camera()
	{
		radius = 10.0f;
		lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
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
		this->cameraPos = tempVec + lookAt;

		this->upDir = glm::normalize(glm::cross(currPos, UpRotAxis));
		this->lookDir = glm::normalize(this->lookAt - this->cameraPos);

		viewMatrix = glm::lookAt(cameraPos, cameraPos + lookDir, upDir);
		return viewMatrix;
	}
};

struct FirstPersonCamera : public Camera
{
	FirstPersonCamera() : Camera(){}
	~FirstPersonCamera(){}

	void pan(float x, float y)
	{
		glm::vec3 right = glm::normalize(glm::cross(this->lookDir,this->upDir));
		glm::vec3 up = this->upDir;
		glm::vec3 moveX = x*right;
		glm::vec3 moveY = y*up;
		this->cameraPos += moveX;
		this->cameraPos += moveY;
	}

	void rotate(float x, float y)
	{
		this->currXZRads += x/3.0f;
		this->currYRads -= y/3.0f;
	}

	void zoom(float distance)
	{
		this->cameraPos += distance*this->lookDir;
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

		viewMatrix = glm::lookAt(cameraPos, cameraPos + lookDir, upDir);
		return viewMatrix;
	}
};