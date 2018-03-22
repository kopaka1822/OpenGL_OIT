#include "DynamicFragmentBuffer.h"
#include <mutex>
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "SimpleShader.h"
#include <iostream>
#include <glad/glad.h>

static const int WORKGROUP_SIZE = 1024;
static const int ELEM_PER_THREAD_SCAN = 8;

DynamicFragmentBufferRenderer::DynamicFragmentBufferRenderer()
{
	// build the shaders
	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");
	auto fragment = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/DefaultShader.fs");
	Program defaultProgram;
	defaultProgram.attach(vertex).attach(geometry).attach(fragment).link();
	m_defaultShader = std::make_unique<SimpleShader>(std::move(defaultProgram));

	auto countFragments = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/DynamicCountFragment.fs");
	auto storeFragments = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/DynamicStoreFragment.fs");
	auto sortBlendShader = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/DynamicSortBlendFragment.fs");

	auto scanShader = Shader::loadFromFile(GL_COMPUTE_SHADER, "Shader/Scan.comp");
	auto pushScanShader = Shader::loadFromFile(GL_COMPUTE_SHADER, "Shader/ScanPush.comp");

	m_scanShader.attach(scanShader).link();
	m_pushScanShader.attach(pushScanShader).link();

	Program countProgram;
	countProgram.attach(vertex).attach(countFragments).link();

	Program storeProgram;
	storeProgram.attach(vertex).attach(geometry).attach(storeFragments).link();

	m_shaderCountFragments = std::make_unique<SimpleShader>(std::move(countProgram));
	m_shaderStoreFragments = std::make_unique<SimpleShader>(std::move(storeProgram));
	m_shaderSortBlendFragments = std::make_unique<FullscreenQuadShader>(sortBlendShader);

	DynamicFragmentBufferRenderer::onSizeChange(Window::getWidth(), Window::getHeight());

	// staging resource for the fragment count
	m_scanStageBuffer = gl::StaticClientShaderStorageBuffer(sizeof uint32_t);
}

void DynamicFragmentBufferRenderer::render(const IModel* model,const ICamera* camera)
{
	// uniform updates etc.
	if (!model || !camera)
		return;

	m_defaultShader->applyCamera(*camera);
	m_shaderCountFragments->applyCamera(*camera);
	m_shaderStoreFragments->applyCamera(*camera);
	

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

		model->prepareDrawing();
		for (const auto& s : model->getShapes())
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

		auto newSize = m_scanStageBuffer.getElement<uint32_t>(0);

		if (newSize > m_fragmentStorage.getNumElements())
		{
			m_fragmentStorage = gl::DynamicShaderStorageBuffer(8, newSize);
		}
	}

	{
		// store fragments
		std::lock_guard<GpuTimer> g(m_timer[T_STORE_FRAGMENTS]);

		// counter (will be counted down to 0's)
		m_countingBuffer.bind(5);
		// base buffer
		m_auxBuffer.front().bind(6);
		// storage
		m_fragmentStorage.bind(7);

		// colors are still diabled
		model->prepareDrawing();
		for (const auto& s : model->getShapes())
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

// alignment that works if alignment is a power of 2
uint32_t alignPowerOfTwo(uint32_t size, uint32_t alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

void DynamicFragmentBufferRenderer::onSizeChange(int width, int height)
{
	uint32_t alignment = WORKGROUP_SIZE * ELEM_PER_THREAD_SCAN;
	m_curScanSize = alignPowerOfTwo(width * height, alignment);
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
		m_auxBuffer.emplace_back(GLsizei(sizeof uint32_t), GLsizei(alignPowerOfTwo(bs, 4)));
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

	m_scanShader.bind();

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
	m_pushScanShader.bind();
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
