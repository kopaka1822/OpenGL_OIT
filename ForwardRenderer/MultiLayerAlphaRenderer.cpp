#include "MultiLayerAlphaRenderer.h"
#include "SimpleShader.h"
#include <mutex>
#include <glad/glad.h>
#include <glm/detail/func_packing.inl>

static const int SAMPLES_PER_PIXEL = 8;

MultiLayerAlphaRenderer::MultiLayerAlphaRenderer()
{
	// build the shaders
	auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
	auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
	auto fragment = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DefaultShader.fs");
	m_opaqueShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({ vertex, geometry, fragment }));

	auto build = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/MultiLayerAlphaBuild.fs", 450);
	auto resolve = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/MultiLayerAlphaResolve.fs");

	m_transparentShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({ vertex, geometry, build }));

	m_resolveShader = std::make_unique<FullscreenQuadShader>(resolve);

	MultiLayerAlphaRenderer::onSizeChange(Window::getWidth(), Window::getHeight());
}

void MultiLayerAlphaRenderer::render(const IModel* model, const ICamera* camera)
{
	if (!model || !camera)
		return;

	m_opaqueShader->applyCamera(*camera);
	m_transparentShader->applyCamera(*camera);

	{
		std::lock_guard<GpuTimer> g(m_timer[T_OPAQUE]);
		// opaque render pass
		glEnable(GL_DEPTH_TEST);
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		model->prepareDrawing();
		for (const auto& s : model->getShapes())
		{
			if (!s->isTransparent())
				s->draw(m_opaqueShader.get());
		}
	}
	
	{
		std::lock_guard<GpuTimer> g(m_timer[T_CLEAR]);
		
		struct Fragment
		{
			float depth;
			uint32_t color;
		};

		// max depth and (alpha - 1) = 1.0 => alpha = 0.0
		Fragment f = { 
			std::numeric_limits<float>::max(), 
			glm::uintBitsToFloat(glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))) 
		};

		m_storageTex.clear(f, gl::SetDataFormat::RG, gl::SetDataType::FLOAT);
	}

	{
		std::lock_guard<GpuTimer> g(m_timer[T_TRANSPARENT]);

		// bind storage
		m_storageTex.bindAsImage(0, gl::ImageAccess::READ_WRITE);
		// bind the atomic counters
		m_mutexTexture.bindAsImage(1, gl::ImageAccess::READ_WRITE);
		
		// disable colors
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// disable depth write
		glDepthMask(GL_FALSE);

		model->prepareDrawing();
		for (const auto& s : model->getShapes())
		{
			if (s->isTransparent())
			{
				s->draw(m_transparentShader.get());
				//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}
		}

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	{
		std::lock_guard<GpuTimer> g(m_timer[T_RESOLVE]);

		// bind storage as texture
		m_storageTex.bind(7);

		// enable colors
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// set up blending
		glEnable(GL_BLEND);
		// add final color and darken the background
		glBlendFunc(GL_ONE, GL_SRC_ALPHA);

		m_resolveShader->draw();

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
	Profiler::set("transparent", m_timer[T_TRANSPARENT].get());
	Profiler::set("resolve", m_timer[T_RESOLVE].get());
}

void MultiLayerAlphaRenderer::onSizeChange(int width, int height)
{
	m_storageTex = gl::Texture3D(gl::InternalFormat::RG32F, width, height, SAMPLES_PER_PIXEL);

	m_mutexTexture = gl::Texture2D(gl::InternalFormat::R32UI, width, height);
}

