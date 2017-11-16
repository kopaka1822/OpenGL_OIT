#pragma once
#include <vector>
#include <glad/glad.h>
#include <string>

class ElementBuffer
{
public:
	template<class T>
	ElementBuffer(const std::vector<T>& indices)
		: 
	m_numElements(indices.size())
	{
		glGenBuffers(1, &m_id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(T), indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		switch (sizeof(T))
		{
		case 1:
			m_type = GL_UNSIGNED_BYTE;
			break;
		case 2:
			m_type = GL_UNSIGNED_SHORT;
			break;
		case 4:
			m_type = GL_UNSIGNED_INT;
			break;
		default:
			throw std::runtime_error("unexpected size of indice elements: " + std::to_string(sizeof(T)));
		}
	}
	ElementBuffer(const ElementBuffer&) = delete;
	ElementBuffer& operator=(const ElementBuffer&) = delete;
	void swap(ElementBuffer& o) noexcept
	{
		std::swap(o.m_id, m_id);
		std::swap(o.m_numElements, m_numElements);
		std::swap(o.m_type, m_type);
	}
	ElementBuffer(ElementBuffer&& o) noexcept
	{
		swap(o);
	}
	ElementBuffer& operator=(ElementBuffer&& o) noexcept
	{
		swap(o);
		return *this;
	}
	~ElementBuffer()
	{
		if (m_id)
			glDeleteBuffers(1, &m_id);
	}

	void drawElements() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_numElements), m_type, nullptr);
	}
private:
	GLuint m_id = 0;
	size_t m_numElements;
	GLenum m_type;
};
