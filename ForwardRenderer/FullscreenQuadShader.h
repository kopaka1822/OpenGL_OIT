#pragma once
#include "Graphics/IShader.h"
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "Dependencies/gl/vertexarrayobject.hpp"

/**
 * \brief helper class to draw a fullscreen quad
 */
class FullscreenQuadShader : public IShader
{
public:
	FullscreenQuadShader(const Shader& fragment)
	{
		Shader vert = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/FullscreenQuad.vs");

		m_program.attach(vert).attach(fragment).link();
	}
	
	void bind() const override
	{
		m_program.bind();
	}

	void draw() const
	{
		bind();
		m_vao.bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

private:
	Program m_program;
	gl::VertexArrayObject m_vao;
};
