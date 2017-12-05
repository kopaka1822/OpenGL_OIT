#include "WeightedTransparency.h"

WeightedTransparency::WeightedTransparency()
{
	WeightedTransparency::onSizeChange(Window::getWidth(), Window::getHeight());
}

void WeightedTransparency::render(const IModel * model, IShader * shader, const ICamera * camera)
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
	{
		if (!s->isTransparent())
			s->draw(shader);
		else hasAlpha = true;
	}

	if (!hasAlpha)
		return;

	m_accumTexture->clear(glm::vec4(0.0f));
	m_revealageTexture->clear(glm::vec4(1.0f));


}

void WeightedTransparency::onSizeChange(int width, int height)
{
	m_accumTexture.reset(new Texture2D(GL_RGBA32F, GL_RGBA, width, height, GL_FLOAT, false, nullptr));
	m_revealageTexture.reset(new Texture2D(GL_RGBA32F, GL_RGBA, width, height, GL_FLOAT, false, nullptr));

	m_framebuffer = std::make_unique<Framebuffer>();
	m_framebuffer->attachColorTarget(*m_accumTexture, 0);
	m_framebuffer->attachColorTarget(*m_revealageTexture, 1);
	m_framebuffer->validate();
}
