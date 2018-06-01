#include "ShadowDebugRenderer.h"

ShadowDebugRenderer::ShadowDebugRenderer()
{
	auto frag = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/ShadowDebug.fs");

	m_shader = std::make_unique<FullscreenQuadShader>(frag);
}

void ShadowDebugRenderer::render(const RenderArgs & args)
{
	if (!args.shadows) return;
	args.shadows->bind();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	m_shader->draw();
}
