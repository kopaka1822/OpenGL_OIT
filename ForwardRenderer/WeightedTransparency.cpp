#include "WeightedTransparency.h"
#include "SimpleShader.h"

WeightedTransparency::WeightedTransparency()
{
	auto combineShader = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/WeightedCombine.fs");
	m_quadShader = std::make_unique<FullscreenQuadShader>(combineShader);

	auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
	auto transShader = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/WeightedTransparent.fs");

	Program transProgram;
	transProgram.attach(vertex).attach(transShader).link();

	m_transShader = std::make_unique<SimpleShader>(std::move(transProgram));

	WeightedTransparency::onSizeChange(Window::getWidth(), Window::getHeight());
}

void WeightedTransparency::render(const IModel * model, IShader * shader, const ICamera * camera)
{
	if (!model || !shader || !camera)
		return;

	m_opaqueFramebuffer->bind();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->applyCamera(*camera);

	model->prepareDrawing();
	for (const auto& s : model->getShapes())
	{
		if (!s->isTransparent())
			s->draw(shader);
	}

	m_transparentFramebuffer->bind();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

	model->prepareDrawing();
	for (const auto& s : model->getShapes())
	{
		if (s->isTransparent())
			s->draw(shader);
	}

	glDisable(GL_BLEND);

	Framebuffer::unbind();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_opaqueTexture->bind(0);
	m_transparentTexture1->bind(1);
	m_transparentTexture2->bind(2);

	glDisable(GL_DEPTH_TEST);
	m_quadShader->draw();
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}

void WeightedTransparency::onSizeChange(int width, int height)
{
	
	m_transparentTexture1.reset(new Texture2D(GL_RGBA32F, GL_RGBA, width, height, GL_FLOAT, false, nullptr));
	m_transparentTexture2.reset(new Texture2D(GL_RGBA32F, GL_RGBA, width, height, GL_FLOAT, false, nullptr));
	m_depthTexture.reset(new Texture2D(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, width, height, GL_FLOAT, false, nullptr));
	m_opaqueTexture.reset(new Texture2D(GL_RGBA8, GL_RGBA, width, height, GL_UNSIGNED_BYTE, false, nullptr));

	m_transparentFramebuffer = std::make_unique<Framebuffer>();
	m_transparentFramebuffer->attachDepthTarget(*m_depthTexture);
	m_transparentFramebuffer->attachColorTarget(*m_transparentTexture1, 0);
	m_transparentFramebuffer->attachColorTarget(*m_transparentTexture2, 1);
	m_transparentFramebuffer->validate();
	Framebuffer::unbind();

	m_opaqueFramebuffer = std::make_unique<Framebuffer>();
	m_opaqueFramebuffer->attachDepthTarget(*m_depthTexture);
	m_opaqueFramebuffer->attachColorTarget(*m_opaqueTexture, 0);
	m_opaqueFramebuffer->validate();
	Framebuffer::unbind();
}
