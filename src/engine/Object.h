#pragma once

#include "../Utils.h"
#include "Mesh.h"
#include "ShaderLibrary.h"


struct ObjectPosition
{
    glm::mat4 modelMatrix;
};

struct Object
{    
    ObjectPosition position;
        
    // Test when object has changed. When true, RenderData is updated
    bool dirtyPosition;

    // Mesh that backs this Object
    Mesh* mesh;

    // Object properties
    GLuint shader;

    glm::mat4 scaleMatrix;
    glm::mat4 rotationMatrix;
    glm::mat4 translationMatrix;
    glm::quat rotationQuat;

    uint renderGroupID;
    uint drawCommandID;
    uint globalIndex;


    Object(Mesh* mesh, GLuint shader)
    :
        mesh(mesh),
        shader(shader),
        scaleMatrix(1.0f),
        translationMatrix(1.0f),
        rotationMatrix(1.0f),
        rotationQuat(0.0f, 0.0f, 1.0f, 0.0f),
        dirtyPosition(false)
    {
        updateModelMatrix();
    };

    ~Object(){};


    //-----------------------------
    //    Transformations
    //-----------------------------

    void scale(float amount)
    {
        scaleMatrix[0].x *= amount;
        scaleMatrix[1].y *= amount;
        scaleMatrix[2].z *= amount;

        updateModelMatrix();
    }

    void scale(glm::vec3 amount)
    {
        scaleMatrix[0].x *= amount.x;
        scaleMatrix[1].y *= amount.y;
        scaleMatrix[2].z *= amount.z;
        updateModelMatrix();
    }

    void setScale(float amount)
    {
        scaleMatrix[0].x = amount;
        scaleMatrix[1].y = amount;
        scaleMatrix[2].z = amount;
        updateModelMatrix();
    }

    void setScale(glm::vec3 amount)
    {
        scaleMatrix[0].x = amount.x;
        scaleMatrix[1].y = amount.y;
        scaleMatrix[2].z = amount.z;
        updateModelMatrix();
    }

    glm::vec3 getScale()
    {
        return glm::vec3(scaleMatrix[0][0], scaleMatrix[1][1], scaleMatrix[2][2]);
    }

    void translate(glm::vec3& amount)
    {
        translationMatrix[3].x += amount.x;
        translationMatrix[3].y += amount.y;
        translationMatrix[3].z += amount.z;
        updateModelMatrix();
    }

    void setTranslation(glm::vec3& amount)
    {
        translationMatrix[3].x = amount.x;
        translationMatrix[3].y = amount.y;
        translationMatrix[3].z = amount.z;
        updateModelMatrix();
    }

    glm::vec3 getTranslation()
    {
        return glm::vec3(translationMatrix[3]);
    }

    void rotate(glm::vec3& axis, float angle)
    {
        rotationQuat = glm::rotate(rotationQuat, angle, axis);
        rotationMatrix = glm::mat4_cast(rotationQuat);
        updateModelMatrix();
    }

    void setRotation(glm::vec3& axis, float angle)
    {
        rotationQuat = glm::quat(angle, axis);
        rotationMatrix = glm::mat4_cast(rotationQuat);
        updateModelMatrix();
    }

    void setRotation(glm::mat3& matrix)
    {
        rotationQuat = glm::quat_cast(matrix);
        rotationMatrix = glm::mat4(matrix);
        updateModelMatrix();
    }

    glm::quat getRotation()
    {
        return rotationQuat;
    }

    void updateModelMatrix()
    {
        dirtyPosition = true;
        position.modelMatrix = translationMatrix * scaleMatrix * rotationMatrix;
    }

    //-----------------------------
    //    End Transformations
    //-----------------------------
};
