#include "ShadowDebugRenderer.h"
#include "../Implementations/ShadowMaps.h"

ShadowDebugRenderer::ShadowDebugRenderer()
{
	auto frag = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/ShadowDebug.fs");

	m_shader = std::make_unique<FullscreenQuadShader>(frag);
}

void ShadowDebugRenderer::render(const RenderArgs & args)
{
	if (!args.shadows) return;
	auto shadows = dynamic_cast<const ShadowMaps*>(args.shadows);
	if(!shadows)
	{
		std::cerr << "ShadowMaps are required for this renderer\n";
		return;
	}

	shadows->bindDebug();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	m_shader->draw();
}
