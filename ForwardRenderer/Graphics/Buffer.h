#pragma once
#include <glad/glad.h>
#include <cassert>
#include <glad/glad.h>

class BufferBase
{
public:
	BufferBase(GLenum type, size_t size, const void* data)
		:
	m_type(type),
	m_size(size)
	{
		glGenBuffers(1, &m_id);
		glBindBuffer(m_type, m_id);
		glBufferStorage(m_type, size, data, GL_DYNAMIC_STORAGE_BIT);
	}
	~BufferBase()
	{
		glDeleteBuffers(1, &m_id);
	}
	BufferBase(const BufferBase&) = delete;
	BufferBase& operator=(const BufferBase&) = delete;

	void update(const void* data, GLintptr offset, GLintptr size)
	{
		assert(unsigned(size + offset) <= m_size);
		glBindBuffer(m_type, m_id);
		glBufferSubData(m_type, offset, size, data);
	}

	void update(const void* data)
	{
		update(data, 0, m_size);
	}

	void bind(int bindingIndex) const
	{
		glBindBufferRange(m_type, bindingIndex, m_id, 0, m_size);
		//glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, m_id);
	}

private:
	GLuint m_id = 0;
	GLenum m_type;
	size_t m_size;
};

class UniformBuffer : public BufferBase
{
public:
	explicit UniformBuffer(size_t size, const void* data = nullptr)
		:
	BufferBase(GL_UNIFORM_BUFFER, size, data)
	{}
};

class StorageBuffer : public BufferBase
{
public:
	explicit StorageBuffer(size_t size, const void* data = nullptr)
		:
	BufferBase(GL_SHADER_STORAGE_BUFFER, size, data)
	{}
};
