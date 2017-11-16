#pragma once
#include "Shader.h"

class Program
{
public:
	Program();
	Program(const Program&) = delete;
	Program& operator=(const Program&) = delete;
	Program& attach(const Shader& shader);
	Program& link();
	void bind() const;

private:
	GLuint m_id = 0;
};