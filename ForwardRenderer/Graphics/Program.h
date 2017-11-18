#pragma once
#include "Shader.h"

class Program
{
public:
	Program();
	Program(const Program&) = delete;
	Program& operator=(const Program&) = delete;
	void swap(Program& o) noexcept
	{
		std::swap(m_id, o.m_id);
	}
	Program(Program&& o) noexcept
	{
		swap(o);
	}
	Program& operator=(Program&& o) noexcept
	{
		swap(o);
		return *this;
	}

	Program& attach(const Shader& shader);
	Program& link();
	void bind() const;

private:
	GLuint m_id = 0;
};