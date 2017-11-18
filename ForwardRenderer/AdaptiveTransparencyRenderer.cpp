#include "AdaptiveTransparencyRenderer.h"
#include "Graphics/Shader.h"
#include "Graphics/Program.h"
#include "SimpleShader.h"

AdaptiveTransparencyRenderer::AdaptiveTransparencyRenderer()
{
	// build the shaders
	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	// geometry build normals if they are not present
	auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");
	auto buildVisz = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/AdaptiveBuildVisibility.fs");

	Program buildProgram;
	buildProgram.attach(vertex).attach(buildVisz).link();

	m_shaderBuildVisz = std::make_unique<SimpleShader>(std::move(buildProgram));

	m_shaderApplyVisz = std::make_unique<SimpleShader>(SimpleShader::getLinkedDefaultProgram());
}

void AdaptiveTransparencyRenderer::render(const IModel* model, IShader* shader, const ICamera* camera)
{
	if (!model || !shader || !camera)
		return;

	glEnable(GL_DEPTH_TEST);
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
	
	// disable depth write
	glDepthMask(GL_FALSE);
	// disable colors
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	model->prepareDrawing();
	for (const auto& s : model->getShapes())
		if (s->isTransparent())
			s->draw(m_shaderBuildVisz.get());

	// enable colors
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	
	// apply visibility function
	// alpha blending + depth buffer read only
	glEnable(GL_BLEND);
	// add all values
	glBlendFunc(GL_ONE, GL_ONE);
	model->prepareDrawing();
	for (const auto& s : model->getShapes())
		if (s->isTransparent())
			s->draw(m_shaderApplyVisz.get());

	glDisable(GL_BLEND);

	// enable depth write
	glDepthMask(GL_TRUE);
	
}
