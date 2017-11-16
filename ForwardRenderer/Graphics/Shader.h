#pragma once
#include <string>
#include <glad/glad.h>

class Shader
{
public:
	Shader(GLenum type, const std::string& source);
	~Shader();
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	void swap(Shader& other) noexcept
	{
		std::swap(other.m_id, m_id);
		std::swap(other.m_type, m_type);
	}
	Shader(Shader&& rhs) noexcept
	{
		swap(rhs);
	}
	Shader& operator=(Shader&& rhs) noexcept
	{
		swap(rhs);
		return *this;
	}

	static Shader loadFromFile(GLenum type, const std::string& filename);

	GLuint getId() const { return m_id; }
private:
	GLenum m_type;
	GLuint m_id = 0;
};
