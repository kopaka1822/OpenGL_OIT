#pragma once
#include <glad/glad.h>
#include <cassert>
#include <glad/glad.h>

class UniformBuffer
{
public:
	UniformBuffer(size_t size, const void* data = nullptr)
		:
	m_size(size)
	{
		glGenBuffers(1, &m_id);
		glBindBuffer(GL_UNIFORM_BUFFER, m_id);
		glBufferStorage(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_STORAGE_BIT);
	}
	~UniformBuffer()
	{
		glDeleteBuffers(1, &m_id);
	}

	void update(const void* data, GLintptr offset, GLintptr size)
	{
		assert(unsigned(size + offset) <= m_size);
		glBindBuffer(GL_UNIFORM_BUFFER, m_id);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
	}

	void update(const void* data)
	{
		update(data, 0, m_size);
	}

	void bind(int bindingIndex) const
	{
		glBindBufferRange(GL_UNIFORM_BUFFER, bindingIndex, m_id, 0, m_size);
		//glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, m_id);
	}

private:
	GLuint m_id = 0;
	size_t m_size;
};
