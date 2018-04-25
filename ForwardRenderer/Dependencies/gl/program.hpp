#pragma once
#include "shader.hpp"
#include <cassert>
#include <vector>

namespace gl
{
	class Program
	{
		explicit Program(bool)
		{}
	public:
		static Program empty()
		{
			return Program(false);
		}
		Program()
			:
		m_id(glCreateProgram())
		{}
		~Program()
		{
			glDeleteProgram(m_id);
		}
		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
		Program(Program&&) = default;
		Program& operator=(Program&&) = default;

		void bind() const
		{
			assert(m_id);
			glUseProgram(m_id);
		}
		static void unbind()
		{
			glUseProgram(0);
		}

		Program& attach(const Shader& shader)
		{
			assert(m_id);
			assert(shader.getId());
			glAttachShader(m_id, shader.getId());
			m_attachments.push_back(shader.getId());
			return *this;
		}

		Program& link()
		{
			glLinkProgram(m_id);

			// detach attachments
			for (const auto& s : m_attachments)
				glDetachShader(m_id, s);
			m_attachments.clear();

			// test link status
			GLint isLinked = 0;
			glGetProgramiv(m_id, GL_LINK_STATUS, &isLinked);
			if (isLinked == GL_FALSE)
				throw std::runtime_error(getInfoLog());
			
			return *this;
		}

		std::string getInfoLog() const
		{
			GLint length = 0;
			glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
			if (!length)
				return "";

			std::string errorLog;
			errorLog.reserve(length);
			glGetProgramInfoLog(m_id, length, &length, &errorLog[0]);
			return errorLog;
		}

		std::string getBinary() const
		{
			assert(m_id);
			GLint size = 0;
			glGetProgramiv(m_id, GL_PROGRAM_BINARY_LENGTH, &size);
			GLsizei bytesWritten = 0;
			GLenum binaryFormat = 0;
			std::string res;
			res.resize(size);
			glGetProgramBinary(m_id, size, &bytesWritten, &binaryFormat, res.data());

			return res;
		}
	private:
		unique<GLuint> m_id;
		std::vector<GLuint> m_attachments;
	};
}
