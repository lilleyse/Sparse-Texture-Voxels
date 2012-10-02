#pragma once
#include "TextureLibrary.h"
#include "RenderData.h"
#include "../ShaderConstants.h"

// A place to load materials. Contains the textureLibrary.
struct MaterialLibrary
{
    struct MeshMaterial
    {
        glm::vec4 diffuseColor;
        glm::vec4 specularColor;
        glm::ivec2 textureLayer;
        float padding[2];
    };

    struct MaterialData
    {
        std::string materialName;
        glm::vec4 diffuseColor;
        glm::vec4 specularColor;
        std::string diffuseTextureName;

        bool operator==(const MaterialData& other) const
        {
            return materialName == other.materialName;
        }
    };

    TextureLibrary textureLibrary;
    std::vector<MaterialData> materialDataArray;
    std::vector<MeshMaterial> materials;

    // Return the material index
    void addMaterial(MaterialData& materialData)
    {
        // See if this material name has already been added
        std::vector<MaterialData>::iterator foundMaterial = std::find(materialDataArray.begin(), materialDataArray.end(), materialData);
        if(foundMaterial == materialDataArray.end())
        {
            // This material name has never been added. Create a new material
            MeshMaterial materialGL;
            materialGL.diffuseColor = materialData.diffuseColor;
            materialGL.specularColor = materialData.specularColor;

            if(materialData.diffuseTextureName != "")
            {
                // Get the texture meta data for the diffuse texture
                TextureLibrary::TextureMetaData diffuseTextureMetaData = textureLibrary.addTexture(materialData.diffuseTextureName);
                materialGL.textureLayer.x = diffuseTextureMetaData.textureArrayID;
                materialGL.textureLayer.y = diffuseTextureMetaData.indexInArray;
            }
            else
            {
                materialGL.textureLayer.x = -1;
                materialGL.textureLayer.y = -1;
            }

            materials.push_back(materialGL);
            materialDataArray.push_back(materialData);
        }
    }

    unsigned int getMaterial(std::string& materialName)
    {
        MaterialData materialData;
        materialData.materialName = materialName;
        std::vector<MaterialData>::iterator foundMaterial = std::find(materialDataArray.begin(), materialDataArray.end(), materialData);
        return foundMaterial - materialDataArray.begin();
    }

    void commitToGL(RenderData& renderData)
    {
        if(materials.size() > NUM_MESHES_MAX)
        {
            printf("too many materials! Change the NUM_MESHES_MAX constant");
            return;
        }

        renderData.commitMaterials(&materials[0], sizeof(MeshMaterial)*materials.size(), sizeof(MeshMaterial)*NUM_MESHES_MAX);

        textureLibrary.commitToGL();
    }
};