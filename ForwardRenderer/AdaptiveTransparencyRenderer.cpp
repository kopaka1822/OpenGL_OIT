#include "AdaptiveTransparencyRenderer.h"
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "SimpleShader.h"
#include "Window.h"
#include <iostream>
#include <glad/glad.h>
#include <glad/glad.h>

static const size_t NUM_SMAPLES = 16;

AdaptiveTransparencyRenderer::AdaptiveTransparencyRenderer()
{
	// build the shaders
	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	// geometry build normals if they are not present
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");
	auto buildVisz = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/AdaptiveBuildVisibility.fs");
	auto useVisz = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/AdaptiveUseVisibility.fs");

	Program buildProgram;
	buildProgram.attach(vertex).attach(buildVisz).link();
	Program useProgram;
	useProgram.attach(vertex).attach(geometry).attach(useVisz).link();

	m_shaderBuildVisz = std::make_unique<SimpleShader>(std::move(buildProgram));

	m_shaderApplyVisz = std::make_unique<SimpleShader>(std::move(useProgram));

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
	m_visibilityFunc->update(m_emptyVisibilityFuncData.data());
	
	// bind as image for building func
	m_visibilityFunc->bindAsImage(0, GL_RG32F);
	

	//m_mutexBuffer->update(m_mutexData.data());
	// bind the atomic counters
	//m_mutexBuffer->bind(5);
	m_mutexTexture->bindAsImage(1, GL_R32UI);

	// disable depth write
	glDepthMask(GL_FALSE);
	// disable colors
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	/*auto data = m_visibilityFunc->getImageData<glm::vec2>();
	std::cout << "before function:" << std::endl;
	for (int i = 0; i < m_visibilityFunc->depth(); ++i)
	{
		int idx = i * m_visibilityFunc->depth();
		std::cout << "depth: " << data[idx].x << " trans: " << data[idx].y << std::endl;
	}*/

	model->prepareDrawing();
	int shapeCount = 0;
	for (const auto& s : model->getShapes())
	{
		if (s->isTransparent())
		{
			s->draw(m_shaderBuildVisz.get());
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			glFinish();
			//++shapeCount;
			//auto data = m_mutexTexture->getImageData<GLuint>();
			//auto data = m_mutexBuffer->getData<GLuint>();
			/*for (auto d : data)
			{
				if (d != 0)
					std::cout << "one texel is not one! " << idx << std::endl;
				++idx;
			}*/
			
			/*auto data = m_visibilityFunc->getImageData<glm::vec2>();
			std::cout << "function:" << std::endl;
			int midpoint = m_visibilityFunc->height() / 2 * m_visibilityFunc->width() + m_visibilityFunc->width() / 2;
			for(int i = 0; i < m_visibilityFunc->depth(); ++i)
			{
				int idx = i * m_visibilityFunc->width() * m_visibilityFunc->height() + midpoint;
				std::cout << "depth: " << data[idx].x << " trans: " << data[idx].y << std::endl;
			}*/
			continue;
		}
	}

	
	// enable colors
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	
	
	glDepthMask(GL_FALSE);
	// apply visibility function
	m_visibilityFunc->bind(5);
	// alpha blending + depth buffer read only
	glEnable(GL_BLEND);
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
	//glFinish();
}

void AdaptiveTransparencyRenderer::onSizeChange(int width, int height)
{
	// create visibility function storage
	m_visibilityFunc.reset(new Texture3D(GL_RG32F, GL_RG, width, height, NUM_SMAPLES, GL_FLOAT, false));
	// create data for the empty function
	m_emptyVisibilityFuncData.resize(0);
	m_emptyVisibilityFuncData.assign(width * height * NUM_SMAPLES, glm::vec2(
		std::numeric_limits<float>::max(), // super far away
		1.0f // visibility is still 1
	));

	// create buffer which ensures mutual exclusion
	// 1 entry for each texel + 1 entry for current lock id
	size_t mutexSize = width * height;

	m_mutexData.resize(mutexSize);
	memset(m_mutexData.data(), 0, mutexSize * sizeof(m_mutexData[0]));
	//m_mutexData[0] = width;

	//m_mutexBuffer.reset(new ShaderStorageBuffer(
	//	mutexSize * sizeof(m_mutexData[0]), m_mutexData.data(), GL_DYNAMIC_STORAGE_BIT
	//));

	m_mutexTexture.reset(new Texture2D(GL_R32UI, GL_RED_INTEGER,width, height, GL_UNSIGNED_INT, false,
		m_mutexData.data()
		));
}
