#pragma once

#include <glad/glad.h>

class Texture
{
public:
	GLuint m_id = 0;
	GLsizei m_width, m_height;
	GLenum m_target;
	GLenum m_internalFormat, m_format, m_type;
	bool m_hasMipmaps = false;

	~Texture()
	{
		if (m_id)
			glDeleteTextures(1, &m_id);
	}

	void Resize(GLsizei width, GLsizei height)
	{
		glTextureStorage2D(m_id, 1, m_internalFormat, width, height);
		m_width = width; m_height = width;
		if (m_hasMipmaps)
			glGenerateTextureMipmap(m_id);
	}

	virtual void Bind() = 0;

	void BindUnit(GLuint unit)
	{
		glBindTextureUnit(unit, m_id);
	}

	void GenerateMipmaps()
	{
		glGenerateTextureMipmap(m_id);
	}

	void SetParameterI(GLenum pname, GLint param)
	{
		glTextureParameteri(m_id, pname, param);
	}

};

class Texture2D : public Texture
{
public:
	Texture2D()
	{
		m_target = GL_TEXTURE_2D;
	}
	Texture2D(const std::string& filename)
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &m_id);

		// Load Crosshair Texture
		stbi_uc* data; int width, height, nrChannels;
		if (data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0))
		{
			glTextureStorage2D(m_id, 1, GL_RGBA8, width, height);
			glTextureSubImage2D(m_id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			m_hasMipmaps = true;
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Failed to load texture '" << filename << "'\n";
		}
	}

	Texture2D(GLenum internalformat, GLenum format, GLenum type, GLsizei width, GLsizei height, void* data)
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &m_id);
		glTextureStorage2D(m_id, 1, internalformat, width, height);
		if (data)
			glTextureSubImage2D(m_id, 0, 0, 0, width, height, format, type, data);
	}

	void Bind() override
	{
		glBindTexture(GL_TEXTURE_2D, m_id);
	}

};

class Texture2DArray : public Texture
{
public:
	Texture2DArray()
	{
		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_id);
	}

	void Bind() override
	{
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_id);
	}

};

class Texture3D : public Texture
{
public:
	Texture3D()
	{
		glCreateTextures(GL_TEXTURE_3D, 1, &m_id);
	}

	void Bind() override
	{
		glBindTexture(GL_TEXTURE_3D, m_id);
	}

};
