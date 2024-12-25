#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <string>

class Shader
{
public:
	unsigned int ID;

	// constructor reads and builds the shader
	Shader(const char* vertexPath, const char* fragmentPath);

	void Bind()
	{
		glUseProgram(ID);
	}

	GLint GetUniformLocation(const std::string& name)
	{
		if (uniformLocations.find(name) == uniformLocations.end())
			uniformLocations[name] = glGetUniformLocation(ID, name.c_str());
		return uniformLocations[name];
	}

protected:
	std::unordered_map<std::string, GLint> uniformLocations;
};

class ShaderBinder
{
public:
	ShaderBinder(Shader& shader)
		: ShaderBinder(&shader)
	{ }
	ShaderBinder(Shader* shader)
	{
		m_shader = shader;
		glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &m_oldShaderId);
		if (m_shader->ID != m_oldShaderId)
			glUseProgram(m_shader->ID);
	}

	~ShaderBinder()
	{
		if (m_shader->ID != m_oldShaderId)
			glUseProgram(m_oldShaderId);
	}

	void setBool(const std::string& name, bool value);
	void setBool(GLint loc, bool value);
	void setInt(const std::string& name, int value);
	void setInt(GLint loc, int value);
	void setFloat(const std::string& name, float value);
	void setFloat(GLint loc, float value);
	void setFloat3(const std::string& name, glm::vec3 value);
	void setFloat3(GLint loc, glm::vec3 value);
	void setMat4x4(const std::string& name, glm::mat4x4 value);
	void setMat4x4(GLint loc, glm::mat4x4 value);
	void setMat4x4s(GLint loc, GLsizei count, glm::mat4x4* value);

protected:
	Shader* m_shader;
	GLint m_oldShaderId;
};
