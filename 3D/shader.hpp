#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files in binary mode to avoid encoding issues
            vShaderFile.open(vertexPath, std::ios::binary);
            fShaderFile.open(fragmentPath, std::ios::binary);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            
            // Remove UTF-8 BOM if present (0xEF 0xBB 0xBF)
            if (vertexCode.length() >= 3 && 
                (unsigned char)vertexCode[0] == 0xEF && 
                (unsigned char)vertexCode[1] == 0xBB && 
                (unsigned char)vertexCode[2] == 0xBF) {
                vertexCode = vertexCode.substr(3);
            }
            if (fragmentCode.length() >= 3 && 
                (unsigned char)fragmentCode[0] == 0xEF && 
                (unsigned char)fragmentCode[1] == 0xBB && 
                (unsigned char)fragmentCode[2] == 0xBF) {
                fragmentCode = fragmentCode.substr(3);
            }
            
            // Remove any other BOM variants
            // UTF-16 LE BOM: 0xFF 0xFE
            if (vertexCode.length() >= 2 && 
                (unsigned char)vertexCode[0] == 0xFF && 
                (unsigned char)vertexCode[1] == 0xFE) {
                vertexCode = vertexCode.substr(2);
            }
            if (fragmentCode.length() >= 2 && 
                (unsigned char)fragmentCode[0] == 0xFF && 
                (unsigned char)fragmentCode[1] == 0xFE) {
                fragmentCode = fragmentCode.substr(2);
            }
            // UTF-16 BE BOM: 0xFE 0xFF
            if (vertexCode.length() >= 2 && 
                (unsigned char)vertexCode[0] == 0xFE && 
                (unsigned char)vertexCode[1] == 0xFF) {
                vertexCode = vertexCode.substr(2);
            }
            if (fragmentCode.length() >= 2 && 
                (unsigned char)fragmentCode[0] == 0xFE && 
                (unsigned char)fragmentCode[1] == 0xFF) {
                fragmentCode = fragmentCode.substr(2);
            }
            
            // Clean up: remove any non-ASCII or problematic characters at the start
            size_t startPos = 0;
            while (startPos < vertexCode.length() && startPos < 10) {
                unsigned char c = (unsigned char)vertexCode[startPos];
                if (c == 0x0A || c == 0x0D || c == 0x09 || (c >= 0x20 && c <= 0x7E)) {
                    break;
                }
                startPos++;
            }
            if (startPos > 0) {
                vertexCode = vertexCode.substr(startPos);
            }
            
            startPos = 0;
            while (startPos < fragmentCode.length() && startPos < 10) {
                unsigned char c = (unsigned char)fragmentCode[startPos];
                if (c == 0x0A || c == 0x0D || c == 0x09 || (c >= 0x20 && c <= 0x7E)) {
                    break;
                }
                startPos++;
            }
            if (startPos > 0) {
                fragmentCode = fragmentCode.substr(startPos);
            }
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);

    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use() const
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif

