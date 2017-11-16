#pragma once
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

	void bind(int bindingIndex) const
	{
		//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, m_id);
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingIndex, m_id, 0, m_size);
	}

private:
	GLuint m_id;
	size_t m_size;
};
