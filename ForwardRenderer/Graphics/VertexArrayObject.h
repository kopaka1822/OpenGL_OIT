#pragma once
#include <glad/glad.h>

class VertexArrayObject
{
public:
	VertexArrayObject();
	VertexArrayObject(const VertexArrayObject&) = delete;
	VertexArrayObject& operator=(const VertexArrayObject&) = delete;
	void bind() const;
	void addArray(GLint attributeIndex, GLint bindingIndex, GLint numComponents, GLenum type, GLint offset = 0, GLint divisor = 0);
private:
	GLuint m_id = 0;
};
