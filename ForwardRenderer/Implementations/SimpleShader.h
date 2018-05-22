#pragma once
#include "../Graphics/IShader.h"
#include <memory>
#include "../Dependencies/gl/buffer.hpp"
#include "../Graphics/SamplerCache.h"
#include "../Framework/Window.h"
#include "../Graphics/HotReloadShader.h"

class SimpleShader : public IShader
{
public:
	/**
	 * \brief 
	 * \param program compiled and linked shader program
	 */
	SimpleShader(std::shared_ptr<HotReloadShader::WatchedProgram> program)
		:
	m_program(std::move(program))
	{
		assert(m_program);
	}
	~SimpleShader() override = default;

	void bind() const override
	{
		m_program->getProgram().bind();
	}
private:
	std::shared_ptr<HotReloadShader::WatchedProgram> m_program;
};
