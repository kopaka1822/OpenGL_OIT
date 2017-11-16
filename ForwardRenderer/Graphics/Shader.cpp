#include "Shader.h"
#include <fstream>

Shader::Shader(GLenum type, const std::string& source)
{
	m_id = glCreateShader(type);
	m_type = type;

	// load source
	const char const* src = source.c_str();
	glShaderSource(m_id, 1, &src, nullptr);

	// compile
	glCompileShader(m_id);

	GLint isCompiled = 0;
	glGetShaderiv(m_id, GL_COMPILE_STATUS, &isCompiled);
	if(isCompiled == GL_FALSE)
	{
		// get error message
		GLint length = 0;
		glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &length);
		std::string errorLog;
		errorLog.resize(length);
		glGetShaderInfoLog(m_id, length, &length, &errorLog[0]);
		throw std::runtime_error("failed to compile shader " + std::to_string(m_id) + "\n" + errorLog);
	}
}

Shader::~Shader()
{
	if(m_id != 0)
		glDeleteShader(m_id);
}

Shader Shader::loadFromFile(GLenum type, const std::string& filename)
{
	std::ifstream file;
	file.open(filename);

	std::string output;
	std::string line;
	if (!file.is_open())
		throw std::runtime_error("could not open file " + filename);

	while (file.good())
	{
		std::getline(file, line);
		// TODO add inlcude
		output.append(line + "\n");
	}
	return Shader(type, output);
}
