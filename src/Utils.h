#pragma once

// GLEW and GLFW headers
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include <CL/opencl.h>

// GLM libraries
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/half_float.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>

// GLI libraries
#include <gli/gli.hpp>
#include <gli/gtx/loader.hpp>

// STL
#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <vector>
#include <iterator>
#include <string>
#include <cstring>
#include <map>
#include <set>
#include <utility>
#include <limits>
#include <algorithm>

typedef unsigned char uchar;
typedef unsigned int uint;

const std::string SOURCE_DIRECTORY = std::string("src/");
const std::string DATA_DIRECTORY = std::string("data/");
const std::string SHADER_DIRECTORY = std::string(SOURCE_DIRECTORY + "shaders/");
const std::string SCENE_DIRECTORY = std::string(DATA_DIRECTORY + "scenes/");
const std::string IMAGE_DIRECTORY = std::string(DATA_DIRECTORY + "images/");
const std::string MESH_DIRECTORY = std::string(DATA_DIRECTORY + "meshes/");

namespace Utils
{
    std::string loadFile(std::string const & Filename)
    {
        std::ifstream stream(Filename.c_str(), std::ios::in);

        if(!stream.is_open())
            return "";

        std::string Line = "";
        std::string Text = "";

        while(getline(stream, Line))
            Text += "\n" + Line;

        stream.close();

        return Text;
    }

    int roundToNextPowerOf2(int x)
    {
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return ++x;
    }

    int roundToNextMultiple(int numToRound, int multiple)
    {
        return numToRound == 0 ? 0 : ((numToRound - 1) / multiple + 1) * multiple;
    }

    void printMatrix(glm::mat4& matrix)
    {
        std::cout << "==========PRINTING MATRIX==========" << std::endl;
        std::cout << matrix[0].x << " " << matrix[1].x << " " << matrix[2].x << " " << matrix[3].x << std::endl;
        std::cout << matrix[0].y << " " << matrix[1].y << " " << matrix[2].y << " " << matrix[3].y << std::endl;
        std::cout << matrix[0].z << " " << matrix[1].z << " " << matrix[2].z << " " << matrix[3].z << std::endl;
        std::cout << matrix[0].w << " " << matrix[1].w << " " << matrix[2].w << " " << matrix[3].w << std::endl;
    }

    void printVec3(glm::vec3& vector)
    {
        std::cout << vector[0] << " " << vector[1] << " " << vector[2] << std::endl;
    }

    void printVec4(glm::vec4& vector)
    {
        std::cout << vector[0] << " " << vector[1] << " " << vector[2] << " " << vector[3] << std::endl;
    }

    void printQuat(glm::fquat& quaternion)
    {
        std::cout << quaternion[0] << " " << quaternion[1] << " " << quaternion[2] << " " << quaternion[3] << std::endl;
    }

    std::vector<std::string> parseSpaceSeparatedString(std::string& configData)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(configData);
        std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter<std::vector<std::string> >(tokens));
        return tokens;
    }

    namespace OpenGL
    {
        bool checkProgram(GLuint ProgramName)
        {
            if(!ProgramName)
                return false;

            GLint Result = GL_FALSE;
            glGetProgramiv(ProgramName, GL_LINK_STATUS, &Result);

            fprintf(stdout, "Linking program\n");
            int InfoLogLength;
            glGetProgramiv(ProgramName, GL_INFO_LOG_LENGTH, &InfoLogLength);
            if(InfoLogLength > 0)
            {
                std::vector<char> Buffer(std::max(InfoLogLength, int(1)));
                glGetProgramInfoLog(ProgramName, InfoLogLength, NULL, &Buffer[0]);
                fprintf(stdout, "%s\n", &Buffer[0]);
            }

            return Result == GL_TRUE;
        }

        bool checkShader(GLuint ShaderName, const char* Source)
        {
            if(!ShaderName)
                return false;

            GLint Result = GL_FALSE;
            glGetShaderiv(ShaderName, GL_COMPILE_STATUS, &Result);

            fprintf(stdout, "Compiling shader\n%s...\n", Source);
            int InfoLogLength;
            glGetShaderiv(ShaderName, GL_INFO_LOG_LENGTH, &InfoLogLength);
            if(InfoLogLength > 0)
            {
                std::vector<char> Buffer(InfoLogLength);
                glGetShaderInfoLog(ShaderName, InfoLogLength, NULL, &Buffer[0]);
                fprintf(stdout, "%s\n", &Buffer[0]);
            }

            return Result == GL_TRUE;
        }

        GLuint createShader(GLenum Type, std::string const & Source)
        {
            bool Validated = true;
            GLuint Name = 0;

            if(!Source.empty())
            {
                std::string SourceContent = Utils::loadFile(Source);
                char const * SourcePointer = SourceContent.c_str();
                Name = glCreateShader(Type);
                glShaderSource(Name, 1, &SourcePointer, NULL);
                glCompileShader(Name);
                Validated = Utils::OpenGL::checkShader(Name, SourcePointer);
            }

            return Name;
        }
        bool checkError(const char* Title)
        {
            int Error;
            if((Error = glGetError()) != GL_NO_ERROR)
            {
                std::string ErrorString;
                switch(Error)
                {
                case GL_INVALID_ENUM:
                    ErrorString = "GL_INVALID_ENUM";
                    break;
                case GL_INVALID_VALUE:
                    ErrorString = "GL_INVALID_VALUE";
                    break;
                case GL_INVALID_OPERATION:
                    ErrorString = "GL_INVALID_OPERATION";
                    break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                    ErrorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
                    break;
                case GL_OUT_OF_MEMORY:
                    ErrorString = "GL_OUT_OF_MEMORY";
                    break;
                default:
                    ErrorString = "UNKNOWN";
                    break;
                }
                fprintf(stdout, "OpenGL Error(%s): %s\n", ErrorString.c_str(), Title);
            }
            return Error == GL_NO_ERROR;
        }
        bool checkFramebuffer(GLuint FramebufferName)
        {
            GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            switch(Status)
            {
            case GL_FRAMEBUFFER_UNDEFINED:
                fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_UNDEFINED");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_UNSUPPORTED");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                fprintf(stdout, "OpenGL Error(%s)\n", "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
                break;
            }

            return Status != GL_FRAMEBUFFER_COMPLETE;
        }
        bool checkExtension(char const * String)
        {
            GLint ExtensionCount = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &ExtensionCount);
            for(GLint i = 0; i < ExtensionCount; ++i)
            {
                std::string extensionName = std::string((char const*)glGetStringi(GL_EXTENSIONS, i));
                //printf((extensionName + "\n").c_str());
                if(extensionName == std::string(String))
                    return true;
            }
            return false;
        }
        static void APIENTRY debugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
        {
            char debSource[32], debType[32], debSev[32];
            if(source == GL_DEBUG_SOURCE_API_ARB)
                strcpy(debSource, "OpenGL");
            else if(source == GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB)
                strcpy(debSource, "Windows");
            else if(source == GL_DEBUG_SOURCE_SHADER_COMPILER_ARB)
                strcpy(debSource, "Shader Compiler");
            else if(source == GL_DEBUG_SOURCE_THIRD_PARTY_ARB)
                strcpy(debSource, "Third Party");
            else if(source == GL_DEBUG_SOURCE_APPLICATION_ARB)
                strcpy(debSource, "Application");
            else if(source == GL_DEBUG_SOURCE_OTHER_ARB)
                strcpy(debSource, "Other");
 
            if(type == GL_DEBUG_TYPE_ERROR_ARB)
                strcpy(debType, "error");
            else if(type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB)
                strcpy(debType, "deprecated behavior");
            else if(type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB)
                strcpy(debType, "undefined behavior");
            else if(type == GL_DEBUG_TYPE_PORTABILITY_ARB)
                strcpy(debType, "portability");
            else if(type == GL_DEBUG_TYPE_PERFORMANCE_ARB)
                strcpy(debType, "performance");
            else if(type == GL_DEBUG_TYPE_OTHER_ARB)
                strcpy(debType, "message");
 
            if(severity == GL_DEBUG_SEVERITY_HIGH_ARB)
                strcpy(debSev, "high");
            else if(severity == GL_DEBUG_SEVERITY_MEDIUM_ARB)
                strcpy(debSev, "medium");
            else if(severity == GL_DEBUG_SEVERITY_LOW_ARB)
                strcpy(debSev, "low");

            fprintf(stderr,"%s: %s(%s) %d: %s\n", debSource, debType, debSev, id, message);
        }
    }
}