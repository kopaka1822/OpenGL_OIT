#include "DynamicFragmentBuffer.h"
#include <mutex>
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "SimpleShader.h"

static const int WORKGROUP_SIZE = 1024;
static const int ELEM_PER_THREAD_SCAN = 8;

DynamicFragmentBufferRenderer::DynamicFragmentBufferRenderer()
{
	// build the shaders
	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	// geometry build normals if they are not present
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");
	auto countFragments = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/DynamicCountFragment.fs");

	auto scanShader = Shader::loadFromFile(GL_COMPUTE_SHADER, "Shader/Scan.comp");

	m_scanShader.attach(scanShader).link();

	Program countProgram;
	countProgram.attach(vertex).attach(countFragments).link();

	m_shaderCountFragments = std::make_unique<SimpleShader>(std::move(countProgram));

	DynamicFragmentBufferRenderer::onSizeChange(Window::getWidth(), Window::getHeight());
}

void DynamicFragmentBufferRenderer::render(const IModel* model, IShader* shader, const ICamera* camera)
{
	// uniform updates etc.
	if (!model || !shader || !camera)
		return;

	shader->applyCamera(*camera);
	m_shaderCountFragments->applyCamera(*camera);

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
				s->draw(shader);
		}
	}

	// reset visibility function data
	{
		std::lock_guard<GpuTimer> g(m_timer[T_CLEAR]);
		// TODO use clear in blend stage
		m_auxBuffer.at(0).clear();
	}

	// count fragments
	{
		std::lock_guard<GpuTimer> g(m_timer[T_COUNT_FRAGMENTS]);

		m_auxBuffer.at(0).bind(5);

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
				s->draw(m_shaderCountFragments.get());
			}
		}
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// enable colors
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		// enable depth write
		glDepthMask(GL_TRUE);
	}

	auto data = m_auxBuffer.at(0).getData<uint32_t>();

	// scan
	{
		std::lock_guard<GpuTimer> g(m_timer[T_SCAN]);

		m_scanShader.bind();

		auto bs = m_curScanSize; int i = 0;
		const auto elemPerWk = WORKGROUP_SIZE * ELEM_PER_THREAD_SCAN;
		// Hierarchical scan of blocks
		while (bs > 1)
		{
			m_auxBuffer.at(i).bindAsTextureBuffer(0);
			// shader storage buffer binding
			m_auxBuffer.at(i).bind(0);

			// Bind the auxiliary buffer for the next step or unbind (in the last step)
			if (i + 1 < m_auxBuffer.size())
				// shader storage buffer binding
				m_auxBuffer.at(i + 1).bind(1);
			else glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

			glDispatchCompute((bs + elemPerWk - 1) / elemPerWk, 1, 1);

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			auto data = m_auxBuffer.at(i).getData<uint32_t>();
			if(i + 1 < m_auxBuffer.size())
			{
				auto data2 = m_auxBuffer.at(i + 1).getData<uint32_t>();
				int a = 3;
			}

			bs /= elemPerWk;
			++i;
		}


	}

	{
		// resize
		std::lock_guard<GpuTimer> g(m_timer[T_RESIZE]);

		GLsizei newSize = 100;
		if (newSize > m_fragmentStorage.getNumElements())
		{
			m_fragmentStorage = gl::DynamicShaderStorageBuffer(16, newSize);
		}
	}

	{
		// sort and blend
		std::lock_guard<GpuTimer> g(m_timer[T_SORT]);


	}


	Profiler::set("time", std::accumulate(m_timer.begin(), m_timer.end(), Profiler::Profile(), [](auto time, const GpuTimer& timer)
	{
		return time + timer.get();
	}));
	Profiler::set("clear", m_timer[T_CLEAR].get());
	Profiler::set("opaque", m_timer[T_OPAQUE].get());
	Profiler::set("count_fragments", m_timer[T_COUNT_FRAGMENTS].get());
	Profiler::set("scan", m_timer[T_SCAN].get());
	Profiler::set("opaque", m_timer[T_RESIZE].get());
	Profiler::set("sort", m_timer[T_SORT].get());
}

void DynamicFragmentBufferRenderer::onSizeChange(int width, int height)
{
	uint32_t alignment = WORKGROUP_SIZE * ELEM_PER_THREAD_SCAN;
	m_curScanSize = width * height;
	if (m_curScanSize % alignment != 0)
		m_curScanSize = ((m_curScanSize / alignment) + 1) * alignment;

	uint32_t bs = m_curScanSize;
	while (bs > 1)
	{
		m_auxBuffer.emplace_back(gl::TextureBufferFormat::RGBA32UI, GLsizei(sizeof uint32_t), GLsizei(bs));
		bs /= alignment;
	}
	
}
