#pragma once
#include <glad/glad.h>

class AtomicCounterBuffer
{
public:
	AtomicCounterBuffer(size_t numAtomics)
		:
	m_size(4 * numAtomics)
	{
		glGenBuffers(1, &m_id);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_id);
		glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, m_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
	}

	void bind(GLuint index) const
	{
		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, index, m_id, 0, m_size);
	}

	void clear()
	{
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_id);
		GLuint zero = 0;
		glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
	}

	std::vector<uint32_t> getData()
	{
		std::vector<uint32_t> res;
		res.resize(m_size / sizeof(uint32_t));
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_id);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, m_size, res.data());
		return res;
	}

private:
	GLuint m_id = 0;
	size_t m_size;
};
