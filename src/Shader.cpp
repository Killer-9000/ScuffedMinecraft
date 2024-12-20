#include "Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
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
		// open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
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
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "Error reading shader source files\n";
	}
	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	// 2. compile shaders
	unsigned int vertex, fragment;
	int success;
	char infoLog[512];

	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, nullptr);
	glCompileShader(vertex);
	// print compile errors if any
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
		std::cout << "Error compiling vertex shader!\n" << infoLog << '\n';
	}

	// fragment shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, nullptr);
	glCompileShader(fragment);
	// print compile errors if any
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
		std::cout << "Error compiling fragment shader!\n" << infoLog << '\n';
	}

	// shader program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	// print linking errors if any
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, 512, nullptr, infoLog);
		std::cout << "Error linking shader program!\n" << infoLog << '\n';
	}

	// delete the shaders
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

void Shader::use()
{
	glUseProgram(ID);
}

void Shader::setBool(const std::string& name, bool value)
{
	setBool(GetUniformLocation(name.c_str()), (int)value);
}
void Shader::setBool(GLint loc, bool value)
{
	glUniform1i(loc, (int)value);
}
void Shader::setInt(const std::string& name, int value)
{
	setInt(GetUniformLocation(name.c_str()), value);
}
void Shader::setInt(GLint loc, int value)
{
	glUniform1i(loc, value);
}
void Shader::setFloat(const std::string& name, float value)
{
	setFloat(GetUniformLocation(name.c_str()), value);
}

void Shader::setFloat(GLint loc, float value)
{
	glUniform1f(loc, value);
}

void Shader::setFloat3(const std::string& name, glm::vec3 value)
{
	setFloat3(GetUniformLocation(name.c_str()), value);
}

void Shader::setFloat3(GLint loc, glm::vec3 value)
{
	glUniform3fv(loc, 1, &value.x);
}

void Shader::setMat4x4(const std::string& name, glm::mat4x4 value)
{
	setMat4x4(GetUniformLocation(name.c_str()), value);
}

void Shader::setMat4x4(GLint loc, glm::mat4x4 value)
{
	glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat4x4s(GLint loc, GLsizei count, glm::mat4x4* value)
{
	glUniformMatrix4fv(loc, count, GL_FALSE, (float*)value);
}