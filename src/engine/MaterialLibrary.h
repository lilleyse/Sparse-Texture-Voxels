#pragma once

#include <tinyxml2.h>

#include "Utils.h"
#include "ShaderLibrary.h"
#include "TextureLibrary.h"
#include "RenderData.h"

using namespace tinyxml2;

// A place to load materials. Contains the textureLibrary.
struct MaterialLibrary
{
    struct MeshMaterial
    {
        glm::vec4 diffuseColor;
        glm::vec4 specularColor;
        glm::ivec2 diffuseTexture;
        glm::ivec2 normalTexture;
        glm::ivec2 specularTexture;
        float emission;
        float padding;
    };

    struct MaterialData
    {
        std::string materialName;
        glm::vec4 diffuseColor;
        glm::vec4 specularColor;
        std::string diffuseTextureName;
        std::string normalTextureName;
        std::string specularTextureName;
        float emission;

        bool operator==(const MaterialData& other) const
        {
            return materialName == other.materialName;
        }
    };

    TextureLibrary textureLibrary;
    std::vector<MaterialData> materialDataArray;
    std::vector<MeshMaterial> materials;

    int getMaterial(std::string& materialName)
    {
        MaterialData materialData;
        materialData.materialName = materialName;
        std::vector<MaterialData>::iterator foundMaterial = std::find(materialDataArray.begin(), materialDataArray.end(), materialData);
        if(foundMaterial == materialDataArray.end())
        {
            return -1;
        }
        else
        {
            return foundMaterial - materialDataArray.begin();
        }
        
    }

    // Return the material index
    void addMaterial(MaterialData& materialData)
    {
        // This material name has never been added. Create a new material
        MeshMaterial materialGL;
        materialGL.diffuseColor = materialData.diffuseColor;
        materialGL.specularColor = materialData.specularColor;
        materialGL.diffuseTexture = glm::ivec2(-1);
        materialGL.normalTexture = glm::ivec2(-1);
        materialGL.specularTexture = glm::ivec2(-1);
        materialGL.emission = materialData.emission;

        // Load diffuse texture
        if(materialData.diffuseTextureName != "")
        {
            // Get the texture meta data for the diffuse texture
            TextureLibrary::TextureMetaData diffuseTextureMetaData = textureLibrary.addTexture(materialData.diffuseTextureName);
            materialGL.diffuseTexture.x = diffuseTextureMetaData.textureArrayID;
            materialGL.diffuseTexture.y = diffuseTextureMetaData.indexInArray;
        }

        // Load normal texture
        if(materialData.normalTextureName != "")
        {
            // Get the texture meta data for the diffuse texture
            TextureLibrary::TextureMetaData normalTextureMetaData = textureLibrary.addTexture(materialData.normalTextureName);
            materialGL.normalTexture.x = normalTextureMetaData.textureArrayID;
            materialGL.normalTexture.y = normalTextureMetaData.indexInArray;
        }

        // Load specular texture
        if(materialData.specularTextureName != "")
        {
            // Get the texture meta data for the diffuse texture
            TextureLibrary::TextureMetaData specularTextureMetaData = textureLibrary.addTexture(materialData.specularTextureName);
            materialGL.specularTexture.x = specularTextureMetaData.textureArrayID;
            materialGL.specularTexture.y = specularTextureMetaData.indexInArray;
        }

        materials.push_back(materialGL);
        materialDataArray.push_back(materialData);
    }

    void loadMaterial(XMLElement* materialElement)
    {
        std::string materialName = materialElement->Attribute("name");

        if(getMaterial(materialName) != -1)
        {
            return;
        }

        MaterialLibrary::MaterialData materialData;
        materialData.materialName = materialName;

        // Load diffuse
        XMLElement* diffuseElement = materialElement->FirstChildElement("diffuse");
        if(diffuseElement != 0)
        {
            const char* diffuseTextureName = diffuseElement->Attribute("texture");
            if(diffuseTextureName != 0)
            {
                // Get diffuse texture
                materialData.diffuseTextureName = diffuseTextureName;
                materialData.diffuseColor = glm::vec4(1,0,0,1);
            }
            else
            {
                // Get diffuse color
                const char* diffuseColor = diffuseElement->Attribute("color");
                std::vector<std::string> colorComponents = Utils::parseSpaceSeparatedString(std::string(diffuseColor));
                glm::vec4 color;
                color.r = (float)std::atof(colorComponents[0].c_str());
                color.g = (float)std::atof(colorComponents[1].c_str());
                color.b = (float)std::atof(colorComponents[2].c_str());
                color.a = (float)std::atof(colorComponents[3].c_str());
                materialData.diffuseColor = color;
            }
        }


        // Load normal map
        XMLElement* normalElement = materialElement->FirstChildElement("normal");
        if(normalElement)
        {
            // Get normal texture
            materialData.normalTextureName = normalElement->Attribute("texture");
        }

        // Load specular
        XMLElement* specularElement = materialElement->FirstChildElement("specular");
        if(specularElement != 0)
        {
            const char* specularTextureName = specularElement->Attribute("texture");
            if(specularTextureName != 0)
            {
                // Get specular texture
                materialData.specularTextureName = specularTextureName;
            }
            else
            {
                // Get specular color
                const char* specularColor = specularElement->Attribute("color");
                std::vector<std::string> colorComponents = Utils::parseSpaceSeparatedString(std::string(specularColor));
                glm::vec4 color;
                color.r = (float)std::atof(colorComponents[0].c_str());
                color.g = (float)std::atof(colorComponents[1].c_str());
                color.b = (float)std::atof(colorComponents[2].c_str());
                color.a = (float)std::atof(colorComponents[3].c_str());
                materialData.specularColor = color;
            }
        }

        // Load emission
        XMLElement* emissionElement = materialElement->FirstChildElement("emission");
        if(emissionElement != 0)
        {
            float emissionAmount = (float)std::atof(emissionElement->Attribute("color"));
            materialData.emission = emissionAmount;
        }

        addMaterial(materialData);
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