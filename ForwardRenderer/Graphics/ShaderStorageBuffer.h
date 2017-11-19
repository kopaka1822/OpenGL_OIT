#pragma once
#include <cassert>
#include <glad/glad.h>

class ShaderStorageBuffer
{
public:
	// flags can be: GL_DYNAMIC_STORAGE_BIT, GL_MAP_READ_BIT 
	// GL_MAP_WRITE_BIT, GL_MAP_PERSISTENT_BIT, GL_MAP_COHERENT_BIT, and GL_CLIENT_STORAGE_BIT
	ShaderStorageBuffer(size_t size, const void* data, GLbitfield flags)
		:
	m_size(size)
	{
		glGenBuffers(1, &m_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, data, flags);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
	ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
	ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;

	~ShaderStorageBuffer()
	{
		glDeleteBuffers(1, &m_id);
	}

	void update(const void* data)
	{
		update(data, 0, m_size);
	}

	void update(const void* data, size_t offset, size_t length)
	{
		assert(length - offset <= m_size);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, length, data);
	}

	void bind(int bindingIndex) const
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, m_id);
		//glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingIndex, m_id, 0, m_size);
	}

	template<class T>
	std::vector<T> getData()
	{
		std::vector<T> res;
		res.resize(m_size / sizeof(T));
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_size, res.data());
		return res;
	}

private:
	GLuint m_id;
	size_t m_size;
};
