#include "WeightedTransparency.h"
#include "../Framework/Profiler.h"
#include <numeric>
#include <mutex>
#include "../Implementations/SimpleShader.h"

WeightedTransparency::WeightedTransparency()
{
	auto combineShader = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/WeightedCombine.fs");
	m_quadShader = std::make_unique<FullscreenQuadShader>(combineShader);

	auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
	auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
	auto fragment = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DefaultShader.fs");
	
	m_defaultShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, geometry, fragment}));

	auto transShader = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/WeightedTransparent.fs");

	m_transShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, geometry, transShader}));

	WeightedTransparency::onSizeChange(Window::getWidth(), Window::getHeight());
}

void WeightedTransparency::render(const RenderArgs& args)
{
	if (args.hasNull())
		return;

	args.lights->bind();
	args.transforms->bind();

	{
		std::lock_guard<GpuTimer> g(m_timer[T_OPAQUE]);

		m_opaqueFramebuffer.bind();
		//glClearColor(0.7f, 0.9f, 1.0f, 0.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		args.model->prepareDrawing(*m_defaultShader);
		for (const auto& s : args.model->getShapes())
		{
			if (!s->isTransparent())
				s->draw(m_defaultShader.get());
		}
	}

	{
		std::lock_guard<GpuTimer> g(m_timer[T_BUILD_VIS]);

		m_transparentFramebuffer.bind();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

		args.model->prepareDrawing(*m_transShader);
		for (const auto& s : args.model->getShapes())
		{
			if (s->isTransparent())
				s->draw(m_transShader.get());
		}

		glDisable(GL_BLEND);

		gl::Framebuffer::unbind();
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

	m_transparentFramebuffer = gl::Framebuffer();
	m_transparentFramebuffer.attachDepth(m_depthTexture);
	m_transparentFramebuffer.attachColor(0, m_transparentTexture1);
	m_transparentFramebuffer.attachColor(1, m_transparentTexture2);
	m_transparentFramebuffer.validate();
	gl::Framebuffer::unbind();

	m_opaqueFramebuffer = gl::Framebuffer();
	m_opaqueFramebuffer.attachDepth(m_depthTexture);
	m_opaqueFramebuffer.attachColor(0, m_opaqueTexture);
	m_opaqueFramebuffer.validate();
	gl::Framebuffer::unbind();
}
