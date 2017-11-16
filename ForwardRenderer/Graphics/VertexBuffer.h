#pragma once
#include <vector>
#include <glad/glad.h>

class VertexBuffer
{
public:
	// element stride is the number of elements (T) per vertex
	template<class T>
	VertexBuffer(const std::vector<T>& vertices, size_t elementStride)
		:
		m_size(vertices.size() * sizeof(T)),
		m_stride(elementStride * sizeof(T))
	{
		glGenBuffers(1, &m_id);
		glBindBuffer(GL_ARRAY_BUFFER, m_id);
		glBufferData(GL_ARRAY_BUFFER, m_size, vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	VertexBuffer(const VertexBuffer&) = delete;
	VertexBuffer& operator=(const VertexBuffer&) = delete;
	void swap(VertexBuffer& o) noexcept
	{
		std::swap(o.m_id, m_id);
		std::swap(o.m_size, m_size);
		std::swap(o.m_stride, m_stride);
	}
	VertexBuffer(VertexBuffer&& o) noexcept
	{
		swap(o);
	}
	VertexBuffer& operator=(VertexBuffer&& o) noexcept
	{
		swap(o);
		return *this;
	}
	~VertexBuffer();
	void bind(GLuint bindingIndex) const;
	size_t getNumVertices() const
	{
		return m_size / m_stride;
	}
private:
	GLuint m_id = 0;
	size_t m_size;
	size_t m_stride;
};
