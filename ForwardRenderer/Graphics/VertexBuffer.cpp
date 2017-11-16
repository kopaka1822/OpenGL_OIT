#include "VertexBuffer.h"

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_id);
}

void VertexBuffer::bind(GLuint bindingIndex) const
{
	glBindVertexBuffer(bindingIndex, m_id, 0, static_cast<GLsizei>(m_stride));
}
