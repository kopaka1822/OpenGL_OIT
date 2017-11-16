#include "Program.h"

Program::Program()
{
	m_id = glCreateProgram();
}

Program& Program::attach(const Shader& shader)
{
	glAttachShader(m_id, shader.getId());
	return *this;
}

Program& Program::link()
{
	// link everything
	glLinkProgram(m_id);

	GLint isLinked = 0;
	glGetProgramiv(m_id, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint length = 0;
		glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
		std::string errorLog;
		errorLog.reserve(length);
		glGetProgramInfoLog(m_id, length, &length, &errorLog[0]);
		throw std::runtime_error(errorLog);
	}
	return *this;
}

void Program::bind() const
{
	glUseProgram(m_id);
}
