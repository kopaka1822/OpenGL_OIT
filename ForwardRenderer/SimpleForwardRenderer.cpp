#include "SimpleForwardRenderer.h"
#include <glad/glad.h>
#include <iostream>
#include "Framework/Profiler.h"
#include <mutex>
#include "SimpleShader.h"

SimpleForwardRenderer::SimpleForwardRenderer()
{
	auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
	auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
	auto fragment = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DefaultShader.fs");

	m_defaultShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, geometry, fragment}));
}


SimpleForwardRenderer::~SimpleForwardRenderer()
{
}

void SimpleForwardRenderer::render(const IModel* model, const ICamera* camera)
{
	if (!model || !camera)
		return;

	auto hasAlpha = false;
	{
		std::lock_guard<GpuTimer> g(m_timer[T_OPAQUE]);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		m_defaultShader->applyCamera(*camera);

		model->prepareDrawing(*m_defaultShader);
		for (const auto& s : model->getShapes())
			if (!s->isTransparent())
				s->draw(m_defaultShader.get());
			else hasAlpha = true;
	}
	{
		std::lock_guard<GpuTimer> g(m_timer[T_TRANSPARENT]);
		if (hasAlpha)
		{
			// alpha blending + depth buffer read only
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			model->prepareDrawing(*m_defaultShader);
			for (const auto& s : model->getShapes())
				if (s->isTransparent())
					s->draw(m_defaultShader.get());

			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
		}
	}
	
	Profiler::set("time", std::accumulate(m_timer.begin(), m_timer.end(), Profiler::Profile(), [](auto time, const GpuTimer& timer)
	{
		return time + timer.get();
	}));
	Profiler::set("opaque", m_timer[T_OPAQUE].get());
	Profiler::set("transparent", m_timer[T_TRANSPARENT].get());
}