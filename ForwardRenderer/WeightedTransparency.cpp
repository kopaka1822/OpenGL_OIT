#include "WeightedTransparency.h"
#include "SimpleShader.h"
#include "Framework/Profiler.h"
#include <numeric>
#include <mutex>

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

	{
		std::lock_guard<GpuTimer> g(m_timer[T_OPAQUE]);

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

	{
		std::lock_guard<GpuTimer> g(m_timer[T_BUILD_VIS]);

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

	{
		std::lock_guard<GpuTimer> g(m_timer[T_USE_VIS]);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_opaqueTexture.bind(0);
		m_transparentTexture1.bind(1);
		m_transparentTexture2.bind(2);

		glDisable(GL_DEPTH_TEST);
		m_quadShader->draw();
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}

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
	m_transparentTexture1 = gl::Texture2D(gl::InternalFormat::RGBA16F, width, height);
	m_transparentTexture2 = gl::Texture2D(gl::InternalFormat::R16F, width, height);
	m_depthTexture = gl::Texture2D(gl::InternalFormat::DEPTH_COMPONENT32F, width, height);
	m_opaqueTexture = gl::Texture2D(gl::InternalFormat::RGB8, width, height);

	m_transparentFramebuffer = std::make_unique<Framebuffer>();
	m_transparentFramebuffer->attachDepthTarget(m_depthTexture);
	m_transparentFramebuffer->attachColorTarget(m_transparentTexture1, 0);
	m_transparentFramebuffer->attachColorTarget(m_transparentTexture2, 1);
	m_transparentFramebuffer->validate();
	Framebuffer::unbind();

	m_opaqueFramebuffer = std::make_unique<Framebuffer>();
	m_opaqueFramebuffer->attachDepthTarget(m_depthTexture);
	m_opaqueFramebuffer->attachColorTarget(m_opaqueTexture, 0);
	m_opaqueFramebuffer->validate();
	Framebuffer::unbind();
}
