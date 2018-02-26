#include "LinkedVisibility.h"
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "SimpleShader.h"
#include "Framework/Profiler.h"
#include <numeric>
#include <functional>

static const size_t NODES_PER_PIXEL = 16;

LinkedVisibility::LinkedVisibility()
{
	// build the shaders
	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");

	auto buildVisz = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/LinkedBuildVisibility.fs");
	auto useVisz = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/LinkedUseVisibility.fs");

	auto adjustBg = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/LinkedDarkenBackground.fs");

	Program useProgram;
	useProgram.attach(vertex).attach(geometry).attach(useVisz).link();
	Program buildProgram;
	buildProgram.attach(vertex).attach(buildVisz).link();

	m_shaderBuildVisz = std::make_unique<SimpleShader>(std::move(buildProgram));
	m_shaderApplyVisz = std::make_unique<SimpleShader>(std::move(useProgram));
	m_shaderAdjustBackground = std::make_unique<FullscreenQuadShader>(adjustBg);

	m_counter = std::make_unique<AtomicCounterBuffer>(1);

	LinkedVisibility::onSizeChange(Window::getWidth(), Window::getHeight());
}

void LinkedVisibility::render(const IModel* model, IShader* shader, const ICamera* camera)
{
	if (!model || !shader || !camera)
		return;
	
	shader->applyCamera(*camera);
	m_shaderBuildVisz->applyCamera(*camera);
	m_shaderApplyVisz->applyCamera(*camera);

	m_timer[T_CLEAR].begin();
	{
		m_mutexTexture->clear(uint32_t(0));
		m_counter->clear();
	}
	m_timer[T_CLEAR].end();

	m_timer[T_OPAQUE].begin();
	{
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
		// disable colors
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// disable depth write
		glDepthMask(GL_FALSE);

		model->prepareDrawing();

		m_counter->bind(4);
		m_mutexTexture->bindAsImage(0, GL_R32UI);
		m_buffer->bind(3);

		for (const auto& s : model->getShapes())
		{
			if (s->isTransparent())
			{
				s->draw(m_shaderBuildVisz.get());
			}
		}

		// sync shader storage
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	m_timer[T_BUILD_VIS].end();
	
	m_timer[T_USE_VIS].begin();
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// darken the background
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
		m_shaderAdjustBackground->draw();

		// add all values
		glEnable(GL_BLEND);
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
	m_timer[T_USE_VIS].end();
	
	Profiler::set("time", std::accumulate(m_timer.begin(), m_timer.end(), 0.0, [](auto time, const GpuTimer& timer)
	{
		return time + timer.latest();
	}));
	Profiler::set("clear", m_timer[T_CLEAR].latest());
	Profiler::set("opaque", m_timer[T_OPAQUE].latest());
	Profiler::set("build_vis", m_timer[T_BUILD_VIS].latest());
	Profiler::set("use_vis", m_timer[T_USE_VIS].latest());
}

void LinkedVisibility::onSizeChange(int width, int height)
{
	m_buffer.reset(new ShaderStorageBuffer(width * height * NODES_PER_PIXEL * 12, nullptr, GL_DYNAMIC_STORAGE_BIT));
	m_mutexTexture.reset(new Texture2D(GL_R32UI, GL_RED_INTEGER, width, height, GL_UNSIGNED_INT, false, nullptr));
}