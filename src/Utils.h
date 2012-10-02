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
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <map>
#include <utility>
#include <limits>
#include <sstream>

typedef unsigned char uchar;
typedef unsigned int uint;

namespace Utils
{
    namespace OpenGL
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
                std::string SourceContent = Utils::OpenGL::loadFile(Source);
                char const * SourcePointer = SourceContent.c_str();
                Name = glCreateShader(Type);
                glShaderSource(Name, 1, &SourcePointer, NULL);
                glCompileShader(Name);
                Validated = Utils::OpenGL::checkShader(Name, SourcePointer);
            }

            return Name;
        }
    }
}