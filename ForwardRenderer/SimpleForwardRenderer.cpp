#include "SimpleForwardRenderer.h"
#include <glad/glad.h>

SimpleForwardRenderer::SimpleForwardRenderer()
{
}


SimpleForwardRenderer::~SimpleForwardRenderer()
{
}

void SimpleForwardRenderer::render(const IModel* model, IShader* shader, const ICamera* camera)
{
	if (!model || !shader || !camera)
		return;
	
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->applyCamera(*camera);

	model->prepareDrawing();
	for (const auto& s : model->getShapes())
		s->draw(shader);
}