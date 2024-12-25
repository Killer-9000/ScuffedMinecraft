#pragma once

#include <list>
#include "glad/glad.h"

#pragma pack(push, 1)
struct DrawElementsIndirectCommand
{
	uint32_t count;
	uint32_t instanceCount;
	uint32_t firstIndex;
	uint32_t baseVertex;
	uint32_t baseInstance;
};
#pragma pack(pop)

class Buffer
{
public:
	unsigned int m_id = 0;
	GLenum m_target = 0;
	GLenum m_usage = 0;
	GLsizei m_allocated = 0;

	Buffer(GLenum target)
	{
		m_target = target;
		glCreateBuffers(1, &m_id);
	}

	Buffer(GLenum target, GLsizei size, void* data, GLenum usage)
	{
		m_target = target;
		m_usage = usage;
		m_allocated = size;
		glCreateBuffers(1, &m_id);
		glNamedBufferData(m_id, size, data, usage);
	}
	~Buffer()
	{
		glDeleteBuffers(1, &m_id);
	}

	void Resize(GLsizei newSize, GLenum usage)
	{
		m_usage = usage;
		Resize(newSize);
	}

	void Resize(GLsizei newSize)
	{
		assert(m_usage && "Usage not set, use the other function");

		GLuint tmp;
		glCreateBuffers(1, &tmp);
		glNamedBufferData(tmp, m_allocated, nullptr, GL_STATIC_COPY);
		glCopyNamedBufferSubData(m_id, tmp, 0, 0, m_allocated);
		glNamedBufferData(m_id, newSize, nullptr, m_usage);
		glCopyNamedBufferSubData(tmp, m_id, 0, 0, m_allocated);
		m_allocated = newSize;
		glDeleteBuffers(1, &tmp);
	}

	void SetData(GLsizeiptr size, void* data)
	{
		assert(m_usage && "Usage not set, use the other function");
		if (m_allocated > size)
		{
			glNamedBufferSubData(m_id, 0, size, data);
		}
		else
		{
			m_allocated = size;
			glNamedBufferData(m_id, size, data, m_usage);
		}
	}

	void SetData(GLsizeiptr size, void* data, GLenum usage)
	{
		m_usage = usage;
		if (m_allocated > size)
		{
			glNamedBufferData(m_id, size, data, m_usage);
		}
		else
		{
			m_allocated = size;
			glNamedBufferData(m_id, size, data, m_usage);
		}
	}

	static void ClearBind(GLenum target)
	{
		glBindBuffer(target, 0);
	}

	void Bind(GLenum target = 0)
	{
		glBindBuffer(target ? target : m_target, m_id);
	}
};

class GeoBuffer : public Buffer
{
public:
	struct Node
	{
		GLint offset, size;
	};

	GeoBuffer(GLenum target)
		: Buffer(target)
	{ }

	Node* AddData(size_t size, void* data)
	{
		assert(m_size <= UINT32_MAX && "Added too much data to buffer, overflow.");
		while (m_size + size > m_allocated)
		{
			Resize(m_size + m_size / 2);
		}

		glNamedBufferSubData(m_id, m_size, size, data);
		Node* node = &m_nodes.emplace_back(Node{ (GLint)m_size, (GLint)size });
		m_size += size;
		return node;
	}

	void RemoveData(Node* node)
	{
		for (auto itr = m_nodes.begin(); itr != m_nodes.end();)
		{
			if (itr->offset == node->offset)
			{
				auto itr2 = itr;
				itr++;
				m_nodes.erase(itr2);
			}
		}
	}

	std::list<Node> m_nodes;
	size_t m_size = 0;
};

class BufferBinder
{
public:
	BufferBinder(Buffer& buffer)
		: BufferBinder(&buffer)
	{	}
	BufferBinder(Buffer* buffer)
	{
		m_buffer = buffer;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_oldBuffer);
		if (m_buffer->m_id != m_oldBuffer)
			glBindBuffer(m_buffer->m_target, m_buffer->m_id);
	}
	~BufferBinder()
	{
		if (m_buffer->m_id != m_oldBuffer)
			glBindBuffer(m_buffer->m_target, m_oldBuffer);
	}

protected:
	Buffer* m_buffer;
	GLint m_oldBuffer;
};