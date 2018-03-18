#pragma once
#include "id.h"
#include "../opengl.h"
#include <cassert>
#include "format.h"

namespace gl
{
	class VertexArrayObject
	{
		explicit VertexArrayObject(bool){}
	public:
		static VertexArrayObject empty()
		{
			return VertexArrayObject(false);
		}
		VertexArrayObject()
		{
			glGenVertexArrays(1, &m_id);
		}
		~VertexArrayObject()
		{
			glDeleteVertexArrays(1, &m_id);
		}
		VertexArrayObject(const VertexArrayObject&) = delete;
		VertexArrayObject& operator=(const VertexArrayObject&) = delete;
		VertexArrayObject(VertexArrayObject&&) = default;
		VertexArrayObject& operator=(VertexArrayObject&&) = default;

		void bind() const
		{
			assert(m_id);
			glBindVertexArray(m_id);
		}

		static void unbind()
		{
			glBindVertexArray(0);
		}

		void addAttribute(GLuint attributeIndex, GLuint bindingIndex, VertexType type, GLuint numComponents, GLuint offset = 0, GLuint divisor = 0, bool normalizeInteger = false)
		{
			bind();
			glEnableVertexAttribArray(attributeIndex);
			glVertexAttribDivisor(attributeIndex, divisor);
			
			if (type == VertexType::DOUBLE)
				glVertexAttribLFormat(attributeIndex, numComponents, GLenum(type), offset);
			else if (!normalizeInteger && isIntegerType(type))
				glVertexAttribIFormat(attributeIndex, numComponents, GLenum(type), offset);
			else
				glVertexAttribFormat(attributeIndex, numComponents, GLenum(type), normalizeInteger, offset);

			glVertexAttribBinding(attributeIndex, bindingIndex);
		}
	private:
		unique<GLuint> m_id;
	};
}
