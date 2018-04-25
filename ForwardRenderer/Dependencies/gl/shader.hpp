#pragma once
#include "id.h"
#include "../opengl.h"
#include <string>
#include <regex>
#include <functional>

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
		Shader& compile(GLsizei count, const char* const* source, const char* debugName = "")
		{
			glShaderSource(m_id, count, source, nullptr);

			glCompileShader(m_id);

			GLint isCompiled = 0;
			glGetShaderiv(m_id, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
				throw std::runtime_error("failed to compile shader " + std::string(debugName) + "\n" + getShaderInfoLog());
			
			return *this;
		}

		std::string getShaderInfoLog() const
		{
			GLint length = 0;
			glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &length);
			if (!length)
				return "";

			std::string errorLog;
			errorLog.resize(length);
			glGetShaderInfoLog(m_id, length, &length, &errorLog[0]);
			return errorLog;
		}

		static std::string convertLog(const std::string& log, const std::function<std::string(GLint)>& convertFunction)
		{
			// convert error information with used files table
			// errors are like \n5(20): => error in file 5 line 20
			//const std::regex expr("\n[0-9][0-9]*\\([0-9][0-9]*\\):");
			const std::regex expr("[0-9][0-9]*\\([0-9][0-9]*\\)");
			std::smatch m;

			std::string error;
			std::string remaining = log;

			while (std::regex_search(remaining, m, expr))
			{
				error += m.prefix();

				// append the correct filename
				// extract number
				const auto parOpen = m.str().find('(');
				const auto fileNumber = m.str().substr(0, parOpen);

				const auto num = std::stoi(fileNumber);
				error += convertFunction(GLint(num));
				error += m.str().substr(parOpen);

				remaining = m.suffix().str();
			}

			error += remaining;

			return error;
		}
	private:
		unique<GLuint> m_id;
	};
}
