#include "VertexArrayObject.h"

VertexArrayObject::VertexArrayObject()
{
	glGenVertexArrays(1, &m_id);
}

void VertexArrayObject::bind() const
{
	glBindVertexArray(m_id);
}

void VertexArrayObject::addArray(GLint attributeIndex, GLint bindingIndex, GLint numComponents, GLenum type, GLint offset, GLint divisor)
{
	glBindVertexArray(m_id);
	glEnableVertexAttribArray(attributeIndex);
	glVertexAttribDivisor(attributeIndex, divisor);
	if (type == GL_DOUBLE)
		glVertexAttribLFormat(attributeIndex, numComponents, type, offset);
	else if (type == GL_FLOAT || type == GL_FIXED || type == GL_HALF_FLOAT)
		glVertexAttribFormat(attributeIndex, numComponents, type, GL_TRUE, offset);
	else // assume integer format
		glVertexAttribIFormat(attributeIndex, numComponents, type, offset);

	glVertexAttribBinding(attributeIndex, bindingIndex);
}
