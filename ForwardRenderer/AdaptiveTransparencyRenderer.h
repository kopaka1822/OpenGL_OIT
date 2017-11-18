#pragma once
#include "Graphics/IRenderer.h"
#include "Graphics/IShader.h"

class AdaptiveTransparencyRenderer : public IRenderer
{
public:
	AdaptiveTransparencyRenderer();

	void render(const IModel* model, IShader* shader, const ICamera* camera) override;

private:
	std::unique_ptr<IShader> m_shaderBuildVisz;
	std::unique_ptr<IShader> m_shaderApplyVisz;
};
