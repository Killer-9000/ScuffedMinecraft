#include "Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

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

Shader::Shader()
{

}

Shader::~Shader()
{
	glDeleteProgram(ID);
}

void Shader::VertexShader(const char* path)
{
	int success;
	char infoLog[512];

	auto& code = GetCodeFromPath(path);
	const char* data[] = { code.c_str() };

	shaders[0] = { glCreateShader(GL_VERTEX_SHADER) };
	glShaderSource(shaders[0], 1, data, nullptr);
	glCompileShader(shaders[0]);

	// print compile errors if any
	glGetShaderiv(shaders[0], GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shaders[0], 512, nullptr, infoLog);
		std::cout << "Error compiling vertex shader!\n" << infoLog << '\n';
	}
}

void Shader::FragmentShader(const char* path)
{
	int success;
	char infoLog[512];

	auto& code = GetCodeFromPath(path);
	const char* data[] = { code.c_str() };

	shaders[1] = { glCreateShader(GL_FRAGMENT_SHADER) };
	glShaderSource(shaders[1], 1, data, nullptr);
	glCompileShader(shaders[1]);

	// print compile errors if any
	glGetShaderiv(shaders[1], GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shaders[1], 512, nullptr, infoLog);
		std::cout << "Error compiling fragment shader!\n" << infoLog << '\n';
	}
}

void Shader::ComputeShader(const char* path)
{
	int success;
	char infoLog[512];

	auto& code = GetCodeFromPath(path);
	const char* data[] = { code.c_str() };

	shaders[2] = { glCreateShader(GL_COMPUTE_SHADER) };
	glShaderSource(shaders[2], 1, data, nullptr);
	glCompileShader(shaders[2]);

	// print compile errors if any
	glGetShaderiv(shaders[2], GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shaders[2], 512, nullptr, infoLog);
		std::cout << "Error compiling compute shader!\n" << infoLog << '\n';
	}
}

void Shader::TessalationControlShader(const char* path)
{
	int success;
	char infoLog[512];

	auto& code = GetCodeFromPath(path);
	const char* data[] = { code.c_str() };

	shaders[3] = { glCreateShader(GL_TESS_CONTROL_SHADER) };
	glShaderSource(shaders[3], 1, data, nullptr);
	glCompileShader(shaders[3]);

	// print compile errors if any
	glGetShaderiv(shaders[3], GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shaders[3], 512, nullptr, infoLog);
		std::cout << "Error compiling tessalation control shader!\n" << infoLog << '\n';
	}
}

void Shader::TessalationEvaluationShader(const char* path)
{
	int success;
	char infoLog[512];

	auto& code = GetCodeFromPath(path);
	const char* data[] = { code.c_str() };

	shaders[4] = { glCreateShader(GL_TESS_EVALUATION_SHADER) };
	glShaderSource(shaders[4], 1, data, nullptr);
	glCompileShader(shaders[4]);

	// print compile errors if any
	glGetShaderiv(shaders[4], GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shaders[4], 512, nullptr, infoLog);
		std::cout << "Error compiling compute shader!\n" << infoLog << '\n';
	}
}

bool Shader::Compile()
{
	bool failed = false;
	int success;
	char infoLog[512];

	ID = glCreateProgram();

	for (int i = 0; i < 5; i++)
	{
		if (shaders[i] == UINT32_MAX)
			continue;

		glAttachShader(ID, shaders[i]);
		glLinkProgram(ID);
		// print linking errors if any
		glGetProgramiv(ID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(ID, 512, nullptr, infoLog);
			std::cout << "Error linking shader program!\n" << infoLog << '\n';
			failed = true;
		}

		// delete the shaders, since not needed anymore.
		glDeleteShader(shaders[i]);
		shaders[i] = UINT32_MAX;
	}

	return !failed;
}

const std::string& Shader::GetCodeFromPath(const char* path)
{
	FILE* file;
	file = fopen(path, "rb");

	if (!file)
		return "";

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (length == 0)
	{
		fclose(file);
		return "";
	}

	std::string code(length, '\0');
	fread(code.data(), 1, length, file);

	fclose(file);

	return code;
}

void ShaderBinder::setBool(const std::string& name, bool value)
{
	setBool(m_shader->GetUniformLocation(name), (int)value);
}
void ShaderBinder::setBool(GLint loc, bool value)
{
	glUniform1i(loc, (int)value);
}
void ShaderBinder::setInt(const std::string& name, int value)
{
	setInt(m_shader->GetUniformLocation(name), value);
}
void ShaderBinder::setInt(GLint loc, int value)
{
	glUniform1i(loc, value);
}
void ShaderBinder::setFloat(const std::string& name, float value)
{
	setFloat(m_shader->GetUniformLocation(name), value);
}

void ShaderBinder::setFloat(GLint loc, float value)
{
	glUniform1f(loc, value);
}

void ShaderBinder::setFloat3(const std::string& name, glm::vec3 value)
{
	setFloat3(m_shader->GetUniformLocation(name), value);
}

void ShaderBinder::setFloat3(GLint loc, glm::vec3 value)
{
	glUniform3fv(loc, 1, glm::value_ptr(value));
}

void ShaderBinder::setFloat3s(GLint loc, GLsizei count, glm::vec3* value)
{
	glUniform3fv(loc, count, (float*)value);
}

void ShaderBinder::setMat4x4(const std::string& name, glm::mat4x4 value)
{
	setMat4x4(m_shader->GetUniformLocation(name), value);
}

void ShaderBinder::setMat4x4(GLint loc, glm::mat4x4 value)
{
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderBinder::setMat4x4s(GLint loc, GLsizei count, glm::mat4x4* value)
{
	glUniformMatrix4fv(loc, count, GL_FALSE, (float*)value);
}
