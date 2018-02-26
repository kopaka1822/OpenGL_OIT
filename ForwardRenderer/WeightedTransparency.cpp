#include "WeightedTransparency.h"
#include "SimpleShader.h"
#include "Framework/Profiler.h"
#include <numeric>

WeightedTransparency::WeightedTransparency()
{
	auto combineShader = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/WeightedCombine.fs");
	m_quadShader = std::make_unique<FullscreenQuadShader>(combineShader);

	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	auto transShader = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/WeightedTransparent.fs");
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");

	Program transProgram;
	transProgram.attach(vertex).attach(geometry).attach(transShader).link();

	m_transShader = std::make_unique<SimpleShader>(std::move(transProgram));

	WeightedTransparency::onSizeChange(Window::getWidth(), Window::getHeight());
}

void WeightedTransparency::render(const IModel * model, IShader * shader, const ICamera * camera)
{
	if (!model || !shader || !camera)
		return;

	m_timer[T_OPAQUE].begin();
	{
		m_opaqueFramebuffer->bind();
		//glClearColor(0.7f, 0.9f, 1.0f, 0.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->applyCamera(*camera);
		model->prepareDrawing();
		for (const auto& s : model->getShapes())
		{
			if (!s->isTransparent())
				s->draw(shader);
		}
	}
	m_timer[T_OPAQUE].end();

	m_timer[T_BUILD_VIS].begin();
	{
		m_transparentFramebuffer->bind();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

		m_transShader->applyCamera(*camera);
		model->prepareDrawing();
		for (const auto& s : model->getShapes())
		{
			if (s->isTransparent())
				s->draw(m_transShader.get());
		}

		glDisable(GL_BLEND);

		Framebuffer::unbind();
	}
	m_timer[T_BUILD_VIS].end();

	m_timer[T_USE_VIS].begin();
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_opaqueTexture->bind(0);
		m_transparentTexture1->bind(1);
		m_transparentTexture2->bind(2);

		glDisable(GL_DEPTH_TEST);
		m_quadShader->draw();
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}
	m_timer[T_USE_VIS].end();

	Profiler::set("time", std::accumulate(m_timer.begin(), m_timer.end(), Profiler::Profile(), [](auto time, const GpuTimer& timer)
	{
		return time + timer.get();
	}));
	Profiler::set("opaque", m_timer[T_OPAQUE].get());
	Profiler::set("build_vis", m_timer[T_BUILD_VIS].get());
	Profiler::set("use_vis", m_timer[T_USE_VIS].get());
}

void WeightedTransparency::onSizeChange(int width, int height)
{
	
	m_transparentTexture1.reset(new Texture2D(GL_RGBA32F, GL_RGBA, width, height, GL_FLOAT, false, nullptr));
	m_transparentTexture2.reset(new Texture2D(GL_RGBA32F, GL_RGBA, width, height, GL_FLOAT, false, nullptr));
	m_depthTexture.reset(new Texture2D(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, width, height, GL_FLOAT, false, nullptr));
	m_opaqueTexture.reset(new Texture2D(GL_RGBA8, GL_RGBA, width, height, GL_UNSIGNED_BYTE, false, nullptr));

	m_transparentFramebuffer = std::make_unique<Framebuffer>();
	m_transparentFramebuffer->attachDepthTarget(*m_depthTexture);
	m_transparentFramebuffer->attachColorTarget(*m_transparentTexture1, 0);
	m_transparentFramebuffer->attachColorTarget(*m_transparentTexture2, 1);
	m_transparentFramebuffer->validate();
	Framebuffer::unbind();

	m_opaqueFramebuffer = std::make_unique<Framebuffer>();
	m_opaqueFramebuffer->attachDepthTarget(*m_depthTexture);
	m_opaqueFramebuffer->attachColorTarget(*m_opaqueTexture, 0);
	m_opaqueFramebuffer->validate();
	Framebuffer::unbind();
}
