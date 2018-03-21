#pragma once
#include "id.h"
#include "../opengl.h"
#include <string>

namespace gl
{
	class Shader
	{
	public:
		enum class Type
		{
			VERTEX = GL_VERTEX_SHADER,
			FRAGMENT = GL_FRAGMENT_SHADER,
			TESS_EVALUATION = GL_TESS_EVALUATION_SHADER,
			TESS_CONTROL = GL_TESS_CONTROL_SHADER,
			GEOMETRY = GL_GEOMETRY_SHADER,
			COMPUTE = GL_COMPUTE_SHADER
		};

		Shader() = default;
		explicit Shader(Type type)
			:
		m_id(glCreateShader(GLenum(type)))
		{}
		~Shader()
		{
			glDeleteShader(m_id);
		}
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;
		Shader(Shader&&) = default;
		Shader& operator=(Shader&&) = default;

		GLuint getId() const
		{
			return m_id;
		}
		void compile(GLsizei count, const char* const* source, const char* debugName = "")
		{
			glShaderSource(m_id, count, source, nullptr);

			glCompileShader(m_id);

			GLint isCompiled = 0;
			glGetShaderiv(m_id, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				// get error message
				GLint length = 0;
				glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &length);
				std::string errorLog;
				errorLog.resize(length);
				glGetShaderInfoLog(m_id, length, &length, &errorLog[0]);
				throw std::runtime_error("failed to compile shader " + std::string(debugName) + "\n" + errorLog);
			}
		}

	private:
		unique<GLuint> m_id;
	};
}
