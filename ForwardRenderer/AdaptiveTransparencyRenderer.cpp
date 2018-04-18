#include "AdaptiveTransparencyRenderer.h"
#include "SimpleShader.h"
#include "Window.h"
#include <iostream>
#include <glad/glad.h>
#include "Framework/Profiler.h"
#include <numeric>
#include <mutex>
#include "ScriptEngine/Token.h"
#include "ScriptEngine/ScriptEngine.h"

AdaptiveTransparencyRenderer::AdaptiveTransparencyRenderer(size_t samplesPerPixel)
	:
m_visibilityClearColor(glm::vec2(
	std::numeric_limits<float>::max(), // super far away
	1.0f // visibility is still 1
)),
m_samplesPerPixel(samplesPerPixel)
{}

AdaptiveTransparencyRenderer::~AdaptiveTransparencyRenderer()
{
	ScriptEngine::removeProperty("adaptive_use_texture");
	ScriptEngine::removeProperty("adaptive_use_texture_buffer_view");
}

void AdaptiveTransparencyRenderer::init()
{
	auto loadShader = [this]()
	{
		std::string shaderParams = "#define MAX_SAMPLES " + std::to_string(m_samplesPerPixel);
		if (!m_useTextureBuffer)
			shaderParams += "\n#define SSBO_STORAGE";
		if (m_useTextureBufferView)
			shaderParams += "\n#define SSBO_TEX_VIEW";

		// build the shaders
		auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
		auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
		auto fragment = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DefaultShader.fs");
		m_defaultShader = std::make_unique<SimpleShader>(
			HotReloadShader::loadProgram({ vertex, geometry, fragment }));

		auto buildVisz = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/AdaptiveBuildVisibility.fs", 450,
			shaderParams
		);

		auto useVisz = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/AdaptiveUseVisibility.fs", 450,
			shaderParams
		);

		auto adjustBg = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/AdaptiveDarkenBackground.fs", 450,
			shaderParams
		);

		m_shaderBuildVisz = std::make_unique<SimpleShader>(
			HotReloadShader::loadProgram({ vertex, buildVisz }));
		m_shaderApplyVisz = std::make_unique<SimpleShader>(
			HotReloadShader::loadProgram({ vertex, geometry, useVisz }));
		m_shaderAdjustBackground = std::make_unique<FullscreenQuadShader>(adjustBg);

		// delete old buffer/texture
		m_visibilityTex = gl::Texture3D();
		m_visibilityBufferView = gl::TextureBuffer();
		m_visibilityBuffer = gl::StaticShaderStorageBuffer();

		AdaptiveTransparencyRenderer::onSizeChange(Window::getWidth(), Window::getHeight());
	};

	loadShader();

	ScriptEngine::addProperty("adaptive_use_texture", [this]()
	{
		std::cout << "adaptive_use_texture: " << m_useTextureBuffer << "\n";
	}, [this, loadShader](const std::vector<Token>& args)
	{
		bool b = args.at(0).getString() != "false";
		// reset timer
		m_timer = std::array<GpuTimer, SIZE>();
		m_useTextureBuffer = b;
		std::cout << "adaptive_use_texture: " << m_useTextureBuffer << "\n";
		loadShader();
	});

	ScriptEngine::addProperty("adaptive_use_texture_buffer_view", [this]()
	{
		std::cout << "adaptive_use_texture_buffer_view: " << m_useTextureBufferView << "\n";
	}, [this, loadShader](const std::vector<Token>& args)
	{
		bool b = args.at(0).getString() != "false";
		// reset timer
		m_timer = std::array<GpuTimer, SIZE>();
		m_useTextureBufferView = b;
		std::cout << "adaptive_use_texture_buffer_view: " << m_useTextureBufferView << "\n";
		loadShader();
	});
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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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

		if (m_useTextureBuffer)
			m_visibilityTex.clear(m_visibilityClearColor, gl::SetDataFormat::RG, gl::SetDataType::FLOAT);
		else
			m_visibilityBuffer.fill(m_visibilityClearColor, gl::InternalFormat::RG32F, gl::SetDataFormat::RG, gl::SetDataType::FLOAT);
	}

	{
		std::lock_guard<GpuTimer> g(m_timer[T_BUILD_VIS]);

		// bind as image for building func
		if (m_useTextureBuffer)
			m_visibilityTex.bindAsImage(0, gl::ImageAccess::READ_WRITE);
		else
			m_visibilityBuffer.bind(7);

		// bind the atomic counters
		m_mutexTexture.bindAsImage(1, gl::ImageAccess::READ_WRITE);

		// disable colors
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// disable depth write
		glDepthMask(GL_FALSE);

		// create Stencil Mask (1's for transparent particles)
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

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

		if (m_useTextureBuffer)
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
		else
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	
	{
		std::lock_guard<GpuTimer> g(m_timer[T_USE_VIS]);
		// enable colors
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// only draw if a 1 is in the stencil buffer
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		// apply visibility function
		if (m_useTextureBuffer)
			m_visibilityTex.bind(7);
		else if (m_useTextureBufferView)
			m_visibilityBufferView.bind(7);
		else
			m_visibilityBuffer.bind(7);

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
		glDisable(GL_STENCIL_TEST);
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
	if (m_useTextureBuffer)
		m_visibilityTex = gl::Texture3D(gl::InternalFormat::RG32F, GLsizei(width), GLsizei(height), GLsizei(m_samplesPerPixel));
	else
	{
		m_visibilityBuffer = gl::StaticShaderStorageBuffer(sizeof(float) * 2, GLsizei(width * height * m_samplesPerPixel));
		m_visibilityBufferView = gl::TextureBuffer(gl::TextureBufferFormat::RG32F, m_visibilityBuffer);
	}

	m_mutexTexture = gl::Texture2D(gl::InternalFormat::R32UI, width, height);
}
