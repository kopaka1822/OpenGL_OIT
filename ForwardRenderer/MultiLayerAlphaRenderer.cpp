#include "MultiLayerAlphaRenderer.h"
#include "SimpleShader.h"
#include <mutex>
#include <glad/glad.h>
#include <glm/detail/func_packing.inl>
#include "ScriptEngine/ScriptEngine.h"
#include <iostream>

static bool s_useTextureBuffer = true;

MultiLayerAlphaRenderer::MultiLayerAlphaRenderer(size_t samplesPerPixel)
	:
m_samplesPerPixel(samplesPerPixel)
{}

MultiLayerAlphaRenderer::~MultiLayerAlphaRenderer()
{
	ScriptEngine::removeProperty("multilayer_use_texture");
}

void MultiLayerAlphaRenderer::init()
{
	auto loadShader = [this]()
	{
		// build the shaders
		auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
		auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
		auto fragment = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DefaultShader.fs");
		m_opaqueShader = std::make_unique<SimpleShader>(
			HotReloadShader::loadProgram({ vertex, geometry, fragment }));

		std::string additionalShaderParams;
		if (!s_useTextureBuffer)
			additionalShaderParams = "\n#define SSBO_STORAGE";

		auto build = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/MultiLayerAlphaBuild.fs", 450,
			"#define MAX_SAMPLES_C " + std::to_string(m_samplesPerPixel)
			+ "\nlayout(location = 10) uniform int MAX_SAMPLES = " + std::to_string(m_samplesPerPixel) + ";"
			+ additionalShaderParams
		);
		auto resolve = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/MultiLayerAlphaResolve.fs", 450,
			"#define MAX_SAMPLES_C " + std::to_string(m_samplesPerPixel)
			+ "\nlayout(location = 10) uniform int MAX_SAMPLES = " + std::to_string(m_samplesPerPixel) + ";"
			+ additionalShaderParams
		);

		m_transparentShader = std::make_unique<SimpleShader>(
			HotReloadShader::loadProgram({ vertex, geometry, build }));

		m_transparentShader->bind();
		glUniform1i(10, GLint(m_samplesPerPixel));

		m_resolveShader = std::make_unique<FullscreenQuadShader>(resolve);

		m_resolveShader->bind();
		glUniform1i(10, GLint(m_samplesPerPixel));

		// delete old buffer/texture
		m_storageTex = gl::Texture3D();
		m_storageBuffer = gl::StaticShaderStorageBuffer();

		MultiLayerAlphaRenderer::onSizeChange(Window::getWidth(), Window::getHeight());
	};

	loadShader();

	ScriptEngine::addProperty("multilayer_use_texture", [this]()
	{
		std::cout << "multilayer_use_texture: " << s_useTextureBuffer << "\n";
	}, [this, loadShader](const std::vector<Token>& args)
	{
		s_useTextureBuffer = args.at(0).getBool();

		// reset timer
		m_timer = std::array<GpuTimer, SIZE>();
		loadShader();
	});
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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		model->prepareDrawing(*m_opaqueShader);
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
			float color;
		};

		// max depth and (alpha - 1) = 1.0 => alpha = 0.0
		Fragment f = { 
			std::numeric_limits<float>::max(), 
			glm::uintBitsToFloat(glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))) 
		};

		

		if (s_useTextureBuffer)
			m_storageTex.clear(f, gl::SetDataFormat::RG, gl::SetDataType::FLOAT);
		else
			m_storageBuffer.fill(f, gl::InternalFormat::RG32F, gl::SetDataFormat::RG, gl::SetDataType::FLOAT);
	}

	{
		std::lock_guard<GpuTimer> g(m_timer[T_TRANSPARENT]);

		// bind storage
		if(s_useTextureBuffer)
			m_storageTex.bindAsImage(0, gl::ImageAccess::READ_WRITE);
		else
			m_storageBuffer.bind(7);

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

		model->prepareDrawing(*m_transparentShader);
		for (const auto& s : model->getShapes())
		{
			if (s->isTransparent())
			{
				s->draw(m_transparentShader.get());
				//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}
		}

		if(s_useTextureBuffer)
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
		else
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		
	}

	{
		std::lock_guard<GpuTimer> g(m_timer[T_RESOLVE]);

		// bind storage as texture
		if(s_useTextureBuffer)
			// TODO bind as texture view
			m_storageTex.bind(7);
		else
			m_storageBuffer.bind(7);

		// enable colors
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// set up blending
		glEnable(GL_BLEND);
		// add final color and darken the background
		glBlendFunc(GL_ONE, GL_SRC_ALPHA);

		// only draw if a 1 is in the stencil buffer
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		m_resolveShader->draw();

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
	Profiler::set("transparent", m_timer[T_TRANSPARENT].get());
	Profiler::set("resolve", m_timer[T_RESOLVE].get());
}

void MultiLayerAlphaRenderer::onSizeChange(int width, int height)
{
	if(s_useTextureBuffer)
		m_storageTex = gl::Texture3D(gl::InternalFormat::RG32F, width, height, GLsizei(m_samplesPerPixel));
	else
		m_storageBuffer = gl::StaticShaderStorageBuffer(sizeof(float) * 2, GLsizei(width * height * m_samplesPerPixel));

	m_mutexTexture = gl::Texture2D(gl::InternalFormat::R32UI, width, height);
}

