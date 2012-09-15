#include <glf.hpp>

// Sparse texture extension function pointer and variables

namespace
{
	std::string const SAMPLE_NAME("Sparse Texture Voxels");
	int const SAMPLE_SIZE_WIDTH(1024);
	int const SAMPLE_SIZE_HEIGHT(768);
	int const SAMPLE_MAJOR_VERSION(4);
	int const SAMPLE_MINOR_VERSION(2);

	glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));

	const int POSITION(0);
	GLuint program;
	GLuint vertexArray;
}

void initGL()
{

	// Debug output
	if(glf::checkExtension("GL_ARB_debug_output"))
	{
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageCallbackARB(&glf::debugOutput, NULL);
	}
	else
	{
		printf("debug output extension not found");
	}

	// Create a test 3D texture
	const unsigned int sideLength = 128;
	GLuint testTexture;
	glGenTextures(1, &testTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, testTexture);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, sideLength, sideLength, sideLength);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Create a gradient of data
	std::vector<glm::u8vec4> textureData(sideLength*sideLength*sideLength);
	for(unsigned int i = 0; i < sideLength; i++)
	{
		for(unsigned int j = 0; j < sideLength; j++)
		{
			for(unsigned int k = 0; k < sideLength; k++)
			{
				textureData[k*sideLength*sideLength + j*sideLength +i] = glm::u8vec4(i, 0, 0, 255);
			}
		}
	}

	// Fill entire texture (first mipmap level)
	glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, sideLength, sideLength, sideLength, GL_RGBA, GL_UNSIGNED_BYTE, &textureData[0]);
	


	// Create buffer objects and vao

	const unsigned int numVertices = 4;
	glm::vec2 vertices[numVertices];
	vertices[0] = glm::vec2(-1.0, -1.0);
	vertices[1] = glm::vec2(1.0, -1.0);
	vertices[2] = glm::vec2(1.0, 1.0);
	vertices[3] = glm::vec2(-1.0, 1.0);

	const unsigned int numElements = 6;
	unsigned short elements[numElements] = {0, 1, 2, 2, 3, 0};

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*numVertices, vertices, GL_STATIC_DRAW);
	
	GLuint elementArrayBuffer;
	glGenBuffers(1, &elementArrayBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*numElements, elements, GL_STATIC_DRAW);

	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glEnableVertexAttribArray(POSITION);
	glVertexAttribPointer(POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);

	//Unbind everything
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	// Create shader program
	GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, "src/simpleShader.vert");
	GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, "src/simpleShader.frag");

	program = glCreateProgram();
	glAttachShader(program, vertexShaderObject);
	glAttachShader(program, fragmentShaderObject);
	glDeleteShader(vertexShaderObject);
	glDeleteShader(fragmentShaderObject);

	glLinkProgram(program);
	glf::checkProgram(program);

	/*
	// Backface culling

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// Enable depth test

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0.0f, 1.0f);
	*/
}

void mouseEvent()
{
	
}

void keyboardEvent(unsigned char keyCode)
{

}

bool begin()
{
	
	initGL();

	return true;
}

bool end()
{
	
	return true;
}

void display()
{
	glUseProgram(program);
	glBindVertexArray(vertexArray);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glf::swapBuffers();
}

int main(int argc, char* argv[])
{
	return glf::run(
		argc, argv,
		glm::ivec2(::SAMPLE_SIZE_WIDTH, ::SAMPLE_SIZE_HEIGHT),
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB, ::SAMPLE_MAJOR_VERSION,
		::SAMPLE_MINOR_VERSION);
}