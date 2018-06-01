#include "LinkedVisibility.h"
#include "../Implementations/SimpleShader.h"
#include "../Framework/Profiler.h"
#include <numeric>
#include <functional>
#include <mutex>

static const size_t NODES_PER_PIXEL = 16;

LinkedVisibility::LinkedVisibility()
{
	// build the shaders
	auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
	auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
	auto fragment = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DefaultShader.fs");
	
	m_defaultShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, geometry, fragment}));

	auto buildVisz = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/LinkedBuildVisibility.fs");
	auto useVisz = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/LinkedUseVisibility.fs");

	auto adjustBg = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/LinkedDarkenBackground.fs");

	m_shaderBuildVisz = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, buildVisz}));
	m_shaderApplyVisz = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, geometry, useVisz}));
	m_shaderAdjustBackground = std::make_unique<FullscreenQuadShader>(adjustBg);

	m_counter = gl::DynamicAtomicCounterBuffer(sizeof GLuint);

	LinkedVisibility::onSizeChange(Window::getWidth(), Window::getHeight());
}

void LinkedVisibility::render(const RenderArgs& args)
{
	if (args.hasNull())
		return;
	
	args.bindLightData();

	{
		std::lock_guard<GpuTimer> g(m_timer[T_CLEAR]);
		m_mutexTexture.clear(uint32_t(0), gl::SetDataFormat::R_INTEGER, gl::SetDataType::UINT32);
		m_counter.clear();
	}
	
	{
		std::lock_guard<GpuTimer> g(m_timer[T_OPAQUE]);

		glEnable(GL_DEPTH_TEST);
		setClearColor();
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
		// disable colors
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// disable depth write
		glDepthMask(GL_FALSE);

		args.model->prepareDrawing(*m_shaderBuildVisz);

		m_counter.bind(4);
		m_mutexTexture.bindAsImage(0, gl::ImageAccess::READ_WRITE);
		m_buffer.bind(3);

		for (const auto& s : args.model->getShapes())
		{
			if (s->isTransparent())
			{
				s->draw(m_shaderBuildVisz.get());
			}
		}

		// sync shader storage
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	
	{
		std::lock_guard<GpuTimer> g(m_timer[T_USE_VIS]);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// darken the background
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
		m_shaderAdjustBackground->draw();

		// add all values
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		args.model->prepareDrawing(*m_shaderApplyVisz);
		for (const auto& s : args.model->getShapes())
		{
			if (s->isTransparent())
			{
				s->draw(m_shaderApplyVisz.get());
			}
		}
		glDisable(GL_BLEND);

		// enable depth write
		glDepthMask(GL_TRUE);
	}
	
	Profiler::set("time", std::accumulate(m_timer.begin(), m_timer.end(), Profiler::Profile(), [](auto time, const GpuTimer& timer)
	{
		return time + timer.get();
	}));
	Profiler::set("clear", m_timer[T_CLEAR].get());
	Profiler::set("opaque", m_timer[T_OPAQUE].get());
	Profiler::set("build_vis", m_timer[T_BUILD_VIS].get());
	Profiler::set("use_vis", m_timer[T_USE_VIS].get());
}

void LinkedVisibility::onSizeChange(int width, int height)
{
	m_buffer = gl::DynamicShaderStorageBuffer(width * height * NODES_PER_PIXEL, 12);
	m_mutexTexture = gl::Texture2D(gl::InternalFormat::R32UI, width, height);
}