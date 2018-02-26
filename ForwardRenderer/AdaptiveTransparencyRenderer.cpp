#include "AdaptiveTransparencyRenderer.h"
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "SimpleShader.h"
#include "Window.h"
#include <iostream>
#include <glad/glad.h>

static const size_t NUM_SMAPLES = 4;

AdaptiveTransparencyRenderer::AdaptiveTransparencyRenderer()
	:
m_visibilityClearColor(glm::vec2(
	std::numeric_limits<float>::max(), // super far away
	1.0f // visibility is still 1
))
{
	// build the shaders
	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	// geometry build normals if they are not present
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");
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

void AdaptiveTransparencyRenderer::render(const IModel* model, IShader* shader, const ICamera* camera)
{
	if (!model || !shader || !camera)
		return;

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_POLYGON_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	shader->applyCamera(*camera);
	m_shaderBuildVisz->applyCamera(*camera);
	m_shaderApplyVisz->applyCamera(*camera);

	// opaque render pass

	
	auto hasAlpha = false;
	model->prepareDrawing();
	for (const auto& s : model->getShapes())
	{
		if (!s->isTransparent())
			s->draw(shader);
		else hasAlpha = true;
	}

	if (!hasAlpha)
		return;
		
	// determine visibility function
	
	// reset visibility function data
	//m_visibilityFunc->update(m_emptyVisibilityFuncData.data());
	m_visibilityFunc->clear(m_visibilityClearColor);

	// bind as image for building func
	m_visibilityFunc->bindAsImage(0, GL_RG32F);
	// bind the atomic counters
	m_mutexTexture->bindAsImage(1, GL_R32UI);

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
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}

	
	// enable colors
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	
	// apply visibility function
	m_visibilityFunc->bind(5);

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

void AdaptiveTransparencyRenderer::onSizeChange(int width, int height)
{
	// create visibility function storage
	m_visibilityFunc.reset(new Texture3D(GL_RG32F, GL_RG, width, height, NUM_SMAPLES, GL_FLOAT, false));

	// create buffer which ensures mutual exclusion
	// 1 entry for each texel + 1 entry for current lock id
	size_t mutexSize = width * height;

	std::vector<uint32_t> mutexData;
	mutexData.resize(mutexSize);
	memset(mutexData.data(), 0, mutexSize * sizeof(mutexData[0]));

	m_mutexTexture.reset(new Texture2D(GL_R32UI, GL_RED_INTEGER,width, height, GL_UNSIGNED_INT, false,
		mutexData.data()
		));
}
