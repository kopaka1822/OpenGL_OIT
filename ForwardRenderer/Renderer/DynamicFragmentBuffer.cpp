#include "DynamicFragmentBuffer.h"
#include <mutex>
#include <iostream>
#include <glad/glad.h>
#include "../ScriptEngine/ScriptEngine.h"
#include "../Framework/alignment.h"
#include "../Implementations/SimpleShader.h"

static const int WORKGROUP_SIZE = 1024;
static const int ELEM_PER_THREAD_SCAN = 8;

DynamicFragmentBufferRenderer::DynamicFragmentBufferRenderer()
{
	// build the shaders
	auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
	auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
	auto fragment = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DefaultShader.fs");

	m_defaultShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, geometry, fragment}));

	auto countFragments = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DynamicCountFragment.fs");
	auto storeFragments = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DynamicStoreFragment.fs");
	auto sortBlendShader = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DynamicSortBlendFragment.fs");

	auto scanShader = HotReloadShader::loadShader(gl::Shader::Type::COMPUTE, "Shader/Scan.comp");
	auto pushScanShader = HotReloadShader::loadShader(gl::Shader::Type::COMPUTE, "Shader/ScanPush.comp");

	m_scanShader = HotReloadShader::loadProgram({ scanShader });
	m_pushScanShader = HotReloadShader::loadProgram({ pushScanShader });

	m_shaderCountFragments = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, countFragments}));
	m_shaderStoreFragments = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({vertex, geometry, storeFragments}));
	m_shaderSortBlendFragments = std::make_unique<FullscreenQuadShader>(sortBlendShader);

	DynamicFragmentBufferRenderer::onSizeChange(Window::getWidth(), Window::getHeight());

	// staging resource for the fragment count
	m_scanStageBuffer = gl::StaticClientShaderStorageBuffer(sizeof uint32_t);
}

DynamicFragmentBufferRenderer::~DynamicFragmentBufferRenderer()
{
	ScriptEngine::removeProperty("dynamic_max_fragments");
	ScriptEngine::removeProperty("dynamic_last_fragments");
}

void DynamicFragmentBufferRenderer::init()
{
	ScriptEngine::addProperty("dynamic_max_fragments", [this]()
	{
		return std::to_string(m_fragmentStorage.getNumElements());
	});

	ScriptEngine::addProperty("dynamic_last_fragments", [this]()
	{
		return std::to_string(m_lastFragmentCount);
	});
}

void DynamicFragmentBufferRenderer::render(const RenderArgs& args)
{
	// uniform updates etc.
	if (args.hasNull())
		return;

	args.transforms->bind();
	args.lights->bind();
	args.environment->bind();

	{
		std::lock_guard<GpuTimer> g(m_timer[T_OPAQUE]);
		// opaque render pass
		glEnable(GL_DEPTH_TEST);
		//glDisable(GL_POLYGON_SMOOTH);
		setClearColor();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		args.model->prepareDrawing(*m_defaultShader);
		for (const auto& s : args.model->getShapes())
		{
			if (!s->isTransparent())
				s->draw(m_defaultShader.get());
		}
	}

	// reset visibility function data
	{
		std::lock_guard<GpuTimer> g(m_timer[T_CLEAR]);
		// nothing needs to be cleard?
	}

	// count fragments
	{
		std::lock_guard<GpuTimer> g(m_timer[T_COUNT_FRAGMENTS]);

		m_countingBuffer.bind(5);

		// disable colors
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		// disable depth write
		glDepthMask(GL_FALSE);

		args.model->prepareDrawing(*m_shaderCountFragments);
		for (const auto& s : args.model->getShapes())
		{
			if (s->isTransparent())
			{
				s->draw(m_shaderCountFragments.get());
			}
		}
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// leave colors disabled
	}
	
	performScan();

	{
		// resize
		std::lock_guard<GpuTimer> g(m_timer[T_RESIZE]);

		m_lastFragmentCount = m_scanStageBuffer.getElement<uint32_t>(0);

		if (m_lastFragmentCount > m_fragmentStorage.getNumElements())
		{
			m_fragmentStorage = gl::DynamicShaderStorageBuffer(8, GLsizei(m_lastFragmentCount));
		}
	}

	{
		// store fragments
		std::lock_guard<GpuTimer> g(m_timer[T_STORE_FRAGMENTS]);
		args.lights->bind();

		// counter (will be counted down to 0's)
		m_countingBuffer.bind(5);
		// base buffer
		m_auxBuffer.front().bind(6);
		// storage
		m_fragmentStorage.bind(7);

		// colors are still diabled
		args.model->prepareDrawing(*m_shaderStoreFragments);
		for (const auto& s : args.model->getShapes())
		{
			if (s->isTransparent())
			{
				s->draw(m_shaderStoreFragments.get());
			}
		}

		// enable colors
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		// TODO is this required?
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	{
		// sort and blend
		std::lock_guard<GpuTimer> g(m_timer[T_SORT]);

		// base buffer for list length determination
		m_auxBuffer.front().bind(6);
		// storage data
		m_fragmentStorage.bind(7);

		// set up blending
		glEnable(GL_BLEND);
		// add final color and darken the background
		glBlendFunc(GL_ONE, GL_SRC_ALPHA);

		m_shaderSortBlendFragments->draw();

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
	Profiler::set("count_fragments", m_timer[T_COUNT_FRAGMENTS].get());
	Profiler::set("scan", m_timer[T_SCAN].get());
	Profiler::set("resize", m_timer[T_RESIZE].get());
	Profiler::set("store_fragments", m_timer[T_STORE_FRAGMENTS].get());
	Profiler::set("sort", m_timer[T_SORT].get());
}

void DynamicFragmentBufferRenderer::onSizeChange(int width, int height)
{
	uint32_t alignment = WORKGROUP_SIZE * ELEM_PER_THREAD_SCAN;
	m_curScanSize = alignPowerOfTwo<uint32_t>(width * height, alignment);
	m_curLastIndex = width * height - 1;

	// buffer for the fragment list length
	m_countingBuffer = gl::StaticShaderStorageBuffer(GLsizei(sizeof uint32_t), GLsizei(alignPowerOfTwo(m_curScanSize, 4)));
	m_countingTextureRGBAView = gl::TextureBuffer(gl::TextureBufferFormat::RGBA32UI, m_countingBuffer);
	// reset to 0's
	m_countingBuffer.clear();

	m_auxBuffer.clear();
	m_auxTextureViews.clear();

	uint32_t bs = m_curScanSize;
	while (bs > 1)
	{
		// buffers for the scan
		m_auxBuffer.emplace_back(GLsizei(sizeof uint32_t), GLsizei(alignPowerOfTwo<uint32_t>(bs, 4)));
		m_auxTextureViews.emplace_back(gl::TextureBufferFormat::RGBA32UI, m_auxBuffer.back());
		bs /= alignment;
	}

	// store screen width for sort blend indexing
	m_shaderSortBlendFragments->bind();
	glUniform1ui(0, width);
}

void DynamicFragmentBufferRenderer::performScan()
{
	std::lock_guard<GpuTimer> g(m_timer[T_SCAN]);

	m_scanShader->getProgram().bind();

	auto bs = m_curScanSize; int i = 0;
	const auto elemPerWk = WORKGROUP_SIZE * ELEM_PER_THREAD_SCAN;
	// Hierarchical scan of blocks
	while (bs > 1)
	{
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		if (i == 0) // in the first step the counting buffer should be used
			m_countingTextureRGBAView.bind(0);
		else
			m_auxTextureViews.at(i).bind(0);

		// shader storage buffer binding
		m_auxBuffer.at(i).bind(0);

		// Bind the auxiliary buffer for the next step or unbind (in the last step)
		if (i + 1 < m_auxBuffer.size())
			// shader storage buffer binding
			m_auxBuffer.at(i + 1).bind(1);
		else glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

		glUniform1ui(0, m_auxBuffer.at(i).getNumElements());
		//glUniform1ui(0, 0);
		glDispatchCompute((bs + elemPerWk - 1) / elemPerWk, 1, 1);

		bs /= elemPerWk;
		++i;
	}

	// Complete Intra-block scan by pushing the values up
	m_pushScanShader->getProgram().bind();
	glUniform1ui(0, elemPerWk);
	glUniform1ui(1, 0);

	m_scanStageBuffer.bind(2);

	--i; bs = m_curScanSize;
	while (bs > elemPerWk) bs /= elemPerWk;
	while (bs < m_curScanSize)
	{
		bs *= elemPerWk;

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		// bind as shader storage
		m_auxBuffer.at(i - 1).bind(1);
		m_auxBuffer.at(i).bind(0);

		if (i == 1) // last write
			glUniform1ui(1, m_curLastIndex);

		glDispatchCompute((bs - elemPerWk) / 64, 1, 1);
		--i;
	}
}
