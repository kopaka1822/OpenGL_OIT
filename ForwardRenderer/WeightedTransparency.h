#pragma once
#include "Graphics/IRenderer.h"
#include "Framework/IWindowReceiver.h"
#include "Graphics/Framebuffer.h"
#include "FullscreenQuadShader.h"
#include "Graphics/GpuTimer.h"
#include <array>

class WeightedTransparency : public IRenderer, public IWindowReceiver
{
public:
	WeightedTransparency();
	void render(const IModel* model, IShader* shader, const ICamera* camera) override;

	void onSizeChange(int width, int height) override;
private:
	std::unique_ptr<Texture2D> m_transparentTexture1;
	std::unique_ptr<Texture2D> m_transparentTexture2;
	std::unique_ptr<Texture2D> m_opaqueTexture;
	std::unique_ptr<Texture2D> m_depthTexture;

	std::unique_ptr<Framebuffer> m_transparentFramebuffer;
	std::unique_ptr<Framebuffer> m_opaqueFramebuffer;
	std::unique_ptr<FullscreenQuadShader> m_quadShader;
	std::unique_ptr<IShader> m_transShader;

	enum Timer
	{
		T_OPAQUE,
		T_BUILD_VIS,
		T_USE_VIS,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;
};
