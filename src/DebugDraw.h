#pragma once

#include <glf.hpp>
#include "Constants.h"

class DebugDraw
{

private:
	GLuint vertexArray;
	GLuint voxelBuffer;
	GLuint voxelDebugProgram;
	static const unsigned int numVerticesCube = 8;
	static const unsigned int numElementsCube = 36; 

	struct MipMapInfo
	{
		unsigned int offset;
		unsigned int numVoxels;
	};

	std::vector<MipMapInfo> mipMapInfoArray;

	struct Voxel
	{
		// .xyz are translation, .w is scale
		glm::vec4 transformation;
		glm::vec4 color;
	};

public:

	DebugDraw()
	{

	}

	void init()
	{
		// Create buffer objects and vao

		
		glm::vec3 vertices[numVerticesCube] = {glm::vec3(-.5, -.5, -.5), glm::vec3(-.5, -.5, .5), glm::vec3(-.5, .5, .5), glm::vec3(-.5, .5, -.5), glm::vec3(.5, .5, -.5), glm::vec3(.5, -.5, -.5), glm::vec3(.5, .5, .5), glm::vec3(.5, -.5, .5)};
		
		unsigned short elements[numElementsCube] = {0, 1, 2, 0, 2, 3, 3, 4, 5, 3, 5, 0, 4, 6, 7, 4, 7, 5, 1, 7, 6, 1, 6, 2, 1, 0, 5, 1, 5, 7, 6, 4, 3, 6, 3, 2};

		GLuint vertexBuffer;
		glGenBuffers(1, &vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*numVerticesCube, vertices, GL_STATIC_DRAW);
	
		GLuint elementArrayBuffer;
		glGenBuffers(1, &elementArrayBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*numElementsCube, elements, GL_STATIC_DRAW);

		glGenBuffers(1, &voxelBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
		// Don't fill this buffer yet

		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);

		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glEnableVertexAttribArray(POSITION_ATTR);
		glVertexAttribPointer(POSITION_ATTR, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
		glEnableVertexAttribArray(DEBUG_TRANSFORM_ATTR);
		glEnableVertexAttribArray(DEBUG_COLOR_ATTR);
		glVertexAttribPointer(DEBUG_TRANSFORM_ATTR, 4, GL_FLOAT, GL_FALSE, sizeof(Voxel), (void*)(0));
		glVertexAttribPointer(DEBUG_COLOR_ATTR, 4, GL_FLOAT, GL_FALSE, sizeof(Voxel), (void*)(sizeof(glm::vec4)));
		glVertexAttribDivisor(DEBUG_TRANSFORM_ATTR, 1);
		glVertexAttribDivisor(DEBUG_COLOR_ATTR, 1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);

		//Unbind everything
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


		// Create shader program
		GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, "src/voxelDebug.vert");
		GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, "src/voxelDebug.frag");

		voxelDebugProgram = glCreateProgram();
		glAttachShader(voxelDebugProgram, vertexShaderObject);
		glAttachShader(voxelDebugProgram, fragmentShaderObject);
		glDeleteShader(vertexShaderObject);
		glDeleteShader(fragmentShaderObject);

		glLinkProgram(voxelDebugProgram);
		glf::checkProgram(voxelDebugProgram);
	}


	void createCubesFromVoxels(GLuint voxelTexture, int textureSideLength, int numMipMaps)
	{
		std::vector<Voxel> voxelArray;

		glBindTexture(GL_TEXTURE_3D, voxelTexture);

		int mipMapSideLength = textureSideLength;
		int voxelScale = 1;
		for(int i = 0; i < numMipMaps; i++)
		{
			MipMapInfo mipMapInfo;
			mipMapInfo.offset = voxelArray.size();
			
			float scale = (float)(voxelScale);
			std::vector<glm::u8vec4> imageData(mipMapSideLength*mipMapSideLength*mipMapSideLength);
			glGetTexImage(GL_TEXTURE_3D, i, GL_RGBA, GL_UNSIGNED_BYTE, &imageData[0]);

			for(int j = 0; j < mipMapSideLength; j++)
			{
				for(int k = 0; k < mipMapSideLength; k++)
				{
					for(int l = 0; l < mipMapSideLength; l++)
					{
						glm::vec3 position(j*scale,k*scale,l*scale);
						unsigned int textureIndex = mipMapSideLength*mipMapSideLength*j + mipMapSideLength*k + l;
						
						glm::u8vec4 color = imageData[textureIndex];
						if(color.a != 0)
						{
							Voxel voxel;
							voxel.color = glm::vec4(color)/255.0f;
							voxel.transformation = glm::vec4(position, scale);
							voxelArray.push_back(voxel);
						}
					}
				}
			}
			mipMapInfo.numVoxels = voxelArray.size() - mipMapInfo.offset;
			mipMapInfoArray.push_back(mipMapInfo);
			voxelScale*=2;
			mipMapSideLength/=2;
			

		}


		glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Voxel)*voxelArray.size(), &voxelArray[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void display(int mipMapLevel)
	{
		unsigned int baseInstance = mipMapInfoArray[mipMapLevel].offset;
		unsigned int primCount = mipMapInfoArray[mipMapLevel].numVoxels;

		glUseProgram(voxelDebugProgram);
		glBindVertexArray(vertexArray);
		glDrawElementsInstancedBaseInstance(GL_TRIANGLES, numElementsCube, GL_UNSIGNED_SHORT, 0, primCount, baseInstance);
	}


};