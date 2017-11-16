#include "VertexBuffer.h"

VertexBuffer::~VertexBuffer()
{
	if(m_id)
		glDeleteBuffers(1, &m_id);
}

void VertexBuffer::bind(GLuint bindingIndex) const
{
	glBindVertexBuffer(bindingIndex, m_id, 0, static_cast<GLsizei>(m_stride));
}