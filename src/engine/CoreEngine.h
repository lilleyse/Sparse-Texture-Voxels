#pragma once

#include "../Utils.h"
#include "Mesh.h"
#include "MeshLibrary.h"
#include "Lighting.h"
#include "Object.h"
#include "Scene.h"
#include "ShaderLibrary.h"
#include "SceneLibrary.h"

class CoreEngine
{
private:

    MeshLibrary meshLibrary;
    ShaderLibrary shaderLibrary;
    RenderData renderData;
    SceneLibrary sceneLibrary;
    Scene* scene;

public:

    void begin()
    {
        shaderLibrary.begin();
        renderData.begin();
        meshLibrary.begin();
        sceneLibrary.begin();

        scene = sceneLibrary.addScene(renderData, meshLibrary, shaderLibrary, "world", SCENE_DIRECTORY + "world.xml");
        meshLibrary.commitToGL(renderData);
        scene->commitToGL();
        renderData.commitToGL();
    }

    void display()
    {
        renderData.display();
    }
};
    
