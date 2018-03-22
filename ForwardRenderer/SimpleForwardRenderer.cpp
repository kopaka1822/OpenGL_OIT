#include "SimpleForwardRenderer.h"
#include <glad/glad.h>
#include <iostream>
#include "Framework/Profiler.h"
#include <mutex>
#include "SimpleShader.h"

SimpleForwardRenderer::SimpleForwardRenderer()
{
	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");
	auto fragment = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/DefaultShader.fs");
	Program defaultProgram;
	defaultProgram.attach(vertex).attach(geometry).attach(fragment).link();
	m_defaultShader = std::make_unique<SimpleShader>(std::move(defaultProgram));
}


SimpleForwardRenderer::~SimpleForwardRenderer()
{
}

void SimpleForwardRenderer::render(const IModel* model, const ICamera* camera)
{
	if (!model || !camera)
		return;
	{
		std::lock_guard<GpuTimer> g(m_timer);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		m_defaultShader->applyCamera(*camera);

		auto hasAlpha = false;
		model->prepareDrawing();
		for (const auto& s : model->getShapes())
			if (!s->isTransparent())
				s->draw(m_defaultShader.get());
			else hasAlpha = true;

		if (hasAlpha)
		{
			// alpha blending + depth buffer read only
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			model->prepareDrawing();
			for (const auto& s : model->getShapes())
				if (s->isTransparent())
					s->draw(m_defaultShader.get());

			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
		}
	}
	Profiler::set("time", m_timer.get());
}