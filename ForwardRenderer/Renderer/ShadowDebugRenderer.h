#pragma once
#include "../Graphics/IRenderer.h"
#include "../Implementations/FullscreenQuadShader.h"

class ShadowDebugRenderer : public IRenderer
{
public:
	ShadowDebugRenderer();

	void render(const RenderArgs& args) override;

private:
	std::unique_ptr<FullscreenQuadShader> m_shader;
};
