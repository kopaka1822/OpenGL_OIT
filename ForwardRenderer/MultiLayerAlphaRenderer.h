#pragma once
#include "Graphics/IRenderer.h"
#include "Framework/IWindowReceiver.h"
#include "FullscreenQuadShader.h"
#include "Graphics/GpuTimer.h"

class MultiLayerAlphaRenderer : public IRenderer, public IWindowReceiver
{
public:
	MultiLayerAlphaRenderer();
	void render(const IModel* model, const ICamera* camera) override;
	void onSizeChange(int width, int height) override;

private:
	std::unique_ptr<IShader> m_opaqueShader;
	std::unique_ptr<IShader> m_transparentShader;
	std::unique_ptr<FullscreenQuadShader> m_resolveShader;

	gl::Texture3D m_storageTex;
	gl::Texture2D m_mutexTexture;

	enum Timer
	{
		T_CLEAR,
		T_OPAQUE,
		T_TRANSPARENT,
		T_RESOLVE,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;
};
