#pragma once
#include "../Graphics/IShader.h"
#include "../Dependencies/gl/vertexarrayobject.hpp"
#include "../Graphics/HotReloadShader.h"
#include <iostream>

/**
 * \brief helper class to draw a fullscreen quad
 */
class FullscreenQuadShader : public IShader
{
public:
	FullscreenQuadShader(const std::shared_ptr<HotReloadShader::WatchedShader>& fragment)
	{
		const auto vert = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/FullscreenQuad.vs");
		m_program = HotReloadShader::loadProgram({vert, fragment});
	}
	FullscreenQuadShader(std::shared_ptr<HotReloadShader::WatchedProgram> program)
		:
	m_program(move(program))
	{}
	
	void bind() const override
	{
		m_program->getProgram().bind();
	}

	void draw() const
	{
		bind();
		m_vao.bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

private:
	std::shared_ptr<HotReloadShader::WatchedProgram> m_program;
	gl::VertexArrayObject m_vao;
};
