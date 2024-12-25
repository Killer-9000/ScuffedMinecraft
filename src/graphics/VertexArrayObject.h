#pragma once

#include <glad/glad.h>
#include "graphics/Buffer.h"

class VertexArrayObject
{
public:
	unsigned int ID;

	VertexArrayObject()
	{
		glCreateVertexArrays(1, &ID);
	}
	~VertexArrayObject()
	{
		glDeleteVertexArrays(1, &ID);
	}

	static void ClearBind()
	{
		glBindVertexArray(0);
	}

	void Bind()
	{
		glBindVertexArray(ID);
	}

	void BindVertexBuffer(GLuint index, Buffer& buffer, GLintptr offset, GLsizei stride)
	{
		glVertexArrayVertexBuffer(ID, index, buffer.m_id, offset, stride);
	}

	void SetAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei offset)
	{
		glEnableVertexArrayAttrib(ID, index);
		glVertexArrayAttribFormat(ID, index, size, type, normalized, offset);
	}

	void SetAttribPointerI(GLuint index, GLint size, GLenum type, GLsizei offset)
	{
		glEnableVertexArrayAttrib(ID, index);
		glVertexArrayAttribIFormat(ID, index, size, type, offset);
	}

};

class VAOBinder
{
public:
	VAOBinder(VertexArrayObject& vao)
		: VAOBinder(&vao)
	{ }
	VAOBinder(VertexArrayObject* vao)
	{
		m_vao = vao;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_oldVao);
		if (m_vao->ID != m_oldVao)
			glBindVertexArray(m_vao->ID);
	}

	~VAOBinder()
	{
		if (m_vao->ID != m_oldVao)
			glBindVertexArray(m_oldVao);
	}

protected:
	VertexArrayObject* m_vao;
	GLint m_oldVao;
};
