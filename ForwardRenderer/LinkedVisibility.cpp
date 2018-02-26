#include "LinkedVisibility.h"
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "SimpleShader.h"
#include "Framework/Profiler.h"

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
	
	m_timer.begin();
	m_mutexTexture->clear(uint32_t(0));
	m_counter->clear();

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->applyCamera(*camera);
	m_shaderBuildVisz->applyCamera(*camera);
	m_shaderApplyVisz->applyCamera(*camera);

	model->prepareDrawing();
	for (const auto& s : model->getShapes())
	{
		if (!s->isTransparent())
			s->draw(shader);
	}


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

	struct BufferData
	{
		float invAlpha;
		float depth;
		uint32_t next;
	};

	//auto counter = m_counter->getData();
	//auto data = m_buffer->getData<BufferData>();
	// enable colors
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
	m_timer.end();
	m_timer.receive();

	Profiler::set("time", m_timer.latest());
}

void LinkedVisibility::onSizeChange(int width, int height)
{
	m_buffer.reset(new ShaderStorageBuffer(width * height * NODES_PER_PIXEL * 12, nullptr, GL_DYNAMIC_STORAGE_BIT));
	m_mutexTexture.reset(new Texture2D(GL_R32UI, GL_RED_INTEGER, width, height, GL_UNSIGNED_INT, false, nullptr));
}