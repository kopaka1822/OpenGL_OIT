#pragma once
#include "Graphics/IRenderer.h"
#include "Framework/IWindowReceiver.h"
#include "Graphics/Framebuffer.h"

class WeightedTransparency : public IRenderer, public IWindowReceiver
{
public:
	WeightedTransparency();
	void render(const IModel* model, IShader* shader, const ICamera* camera) override;

	void onSizeChange(int width, int height) override;
private:
	std::unique_ptr<Texture2D> m_accumTexture;
	std::unique_ptr<Texture2D> m_revealageTexture;
	std::unique_ptr<Framebuffer> m_framebuffer;
};
