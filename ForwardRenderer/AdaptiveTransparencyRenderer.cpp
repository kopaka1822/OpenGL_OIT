#include "AdaptiveTransparencyRenderer.h"
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "SimpleShader.h"
#include "Window.h"
#include <iostream>
#include <glad/glad.h>
#include "Framework/Profiler.h"
#include <numeric>
#include <mutex>

static const size_t NUM_SMAPLES = 16;

AdaptiveTransparencyRenderer::AdaptiveTransparencyRenderer()
	:
m_visibilityClearColor(glm::vec2(
	std::numeric_limits<float>::max(), // super far away
	1.0f // visibility is still 1
))
{
	// build the shaders
	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");
	auto fragment = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/DefaultShader.fs");
	Program defaultProgram;
	defaultProgram.attach(vertex).attach(geometry).attach(fragment).link();
	m_defaultShader = std::make_unique<SimpleShader>(std::move(defaultProgram));

	auto buildVisz = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/AdaptiveBuildVisibility.fs");
	auto useVisz = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/AdaptiveUseVisibility.fs");

	auto adjustBg = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/AdaptiveDarkenBackground.fs");

	Program buildProgram;
	buildProgram.attach(vertex).attach(buildVisz).link();
	Program useProgram;
	useProgram.attach(vertex).attach(geometry).attach(useVisz).link();

	m_shaderBuildVisz = std::make_unique<SimpleShader>(std::move(buildProgram));
	m_shaderApplyVisz = std::make_unique<SimpleShader>(std::move(useProgram));
	m_shaderAdjustBackground = std::make_unique<FullscreenQuadShader>(adjustBg);

	AdaptiveTransparencyRenderer::onSizeChange(Window::getWidth(), Window::getHeight());
}

void AdaptiveTransparencyRenderer::render(const IModel* model, const ICamera* camera)
{
	if (!model || !camera)
		return;

	m_defaultShader->applyCamera(*camera);
	m_shaderBuildVisz->applyCamera(*camera);
	m_shaderApplyVisz->applyCamera(*camera);

	
	{
		std::lock_guard<GpuTimer> g(m_timer[T_OPAQUE]);
		// opaque render pass
		glEnable(GL_DEPTH_TEST);
		//glDisable(GL_POLYGON_SMOOTH);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		model->prepareDrawing();
		for (const auto& s : model->getShapes())
		{
			if (!s->isTransparent())
				s->draw(m_defaultShader.get());
		}
	}
		
	// determine visibility function
	
	// reset visibility function data
	{
		std::lock_guard<GpuTimer> g(m_timer[T_CLEAR]);
		m_visibilityFunc.clear(m_visibilityClearColor, gl::SetDataFormat::RG, gl::SetDataType::FLOAT);
	}

	{
		std::lock_guard<GpuTimer> g(m_timer[T_BUILD_VIS]);

		// bind as image for building func
		m_visibilityFunc.bindAsImage(0, gl::ImageAccess::READ_WRITE);
		// bind the atomic counters
		m_mutexTexture.bindAsImage(1, gl::ImageAccess::READ_WRITE);

		// disable colors
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// disable depth write
		glDepthMask(GL_FALSE);

		model->prepareDrawing();
		int shapeCount = 0;
		for (const auto& s : model->getShapes())
		{
			if (s->isTransparent())
			{
				s->draw(m_shaderBuildVisz.get());
				//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}
		}
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
	
	{
		std::lock_guard<GpuTimer> g(m_timer[T_USE_VIS]);
		// enable colors
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// apply visibility function
		m_visibilityFunc.bind(7);

		// darken the background
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
		m_shaderAdjustBackground->draw();


		// add all values
		glBlendFunc(GL_ONE, GL_ONE);
		model->prepareDrawing();
		for (const auto& s : model->getShapes())
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

void AdaptiveTransparencyRenderer::onSizeChange(int width, int height)
{
	// create visibility function storage
	m_visibilityFunc = gl::Texture3D(gl::InternalFormat::RG32F, width, height, NUM_SMAPLES);

	m_mutexTexture = gl::Texture2D(gl::InternalFormat::R32UI, width, height);
}
