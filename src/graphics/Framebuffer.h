#pragma once

#include <glad/glad.h>
#include <graphics/Texture.h>

class Framebuffer
{
public:
	unsigned int ID;

	Framebuffer()
	{
		glCreateFramebuffers(1, &ID);
	}
	~Framebuffer()
	{
		glDeleteFramebuffers(1, &ID);
	}

	static void ClearBind(GLenum target = GL_FRAMEBUFFER)
	{
		glBindFramebuffer(target, 0);
	}

	void Bind(GLenum target = GL_FRAMEBUFFER)
	{
		glBindFramebuffer(target, ID);
	}

	void SetTexture(GLenum attachment, Texture& texture)
	{
		glNamedFramebufferTexture(ID, attachment, texture.m_id, 0);
	}
	void SetTexture(GLenum attachment, GLuint texture)
	{
		glNamedFramebufferTexture(ID, attachment, texture, 0);
	}
};