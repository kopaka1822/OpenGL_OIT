#pragma once
#include "format.h"
#include "id.h"

namespace gl
{
	class Renderbuffer
	{
	public:
		Renderbuffer() = default;
		Renderbuffer(InternalFormat format, GLsizei width, GLsizei height)
			:
		m_width(width), m_height(height), m_format(format)
		{
			glGenRenderbuffers(1, &m_id);
			bind();
			glRenderbufferStorage(GL_RENDERBUFFER, GLenum(format), width, height);
		}
		~Renderbuffer()
		{
			glDeleteRenderbuffers(1, &m_id);
		}
		Renderbuffer(const Renderbuffer&) = delete;
		Renderbuffer& operator=(const Renderbuffer&) = delete;
		Renderbuffer(Renderbuffer&&) = default;
		Renderbuffer& operator=(Renderbuffer&&) = default;

		void bind() const
		{
			glBindRenderbuffer(GL_RENDERBUFFER, m_id);
		}

		GLsizei width() const
		{
			return m_width;
		}

		GLsizei height() const
		{
			return m_height;
		}

		GLuint getId() const
		{
			return m_id;
		}

		InternalFormat format() const
		{
			return m_format;
		}
	private:
		unique<GLuint> m_id;
		unique<GLsizei> m_width;
		unique<GLsizei> m_height;
		unique<InternalFormat> m_format;
	};
}
