#include "SimpleForwardRenderer.h"
#include <glad/glad.h>
#include <iostream>

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

	auto hasAlpha = false;
	model->prepareDrawing();
	for (const auto& s : model->getShapes())
		if(!s->isTransparent())
			s->draw(shader);
		else hasAlpha = true;

	if(hasAlpha)
	{
		// alpha blending + depth buffer read only
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		model->prepareDrawing();
		for (const auto& s : model->getShapes())
			if (s->isTransparent())
				s->draw(shader);

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}