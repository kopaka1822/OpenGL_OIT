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
	~VertexBuffer();
	void bind(GLuint bindingIndex) const;
private:
	GLuint m_id = 0;
	const size_t m_size;
	const size_t m_stride;
};
