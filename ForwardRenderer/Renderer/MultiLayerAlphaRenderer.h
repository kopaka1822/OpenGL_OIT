#pragma once
#include "../Graphics/IRenderer.h"
#include "../Framework/IWindowReceiver.h"
#include "../Implementations/FullscreenQuadShader.h"
#include "../Graphics/GpuTimer.h"
#include "../Dependencies/gl/buffer.hpp"

#define MULTI_LAYER_SSBO

class MultiLayerAlphaRenderer : public IRenderer, public IWindowReceiver
{
public:
	explicit MultiLayerAlphaRenderer(size_t samplesPerPixel);
	virtual ~MultiLayerAlphaRenderer();

	void init() override;
	void render(const IModel* model, const ICamera* camera, ILights* lights) override;
	void onSizeChange(int width, int height) override;

private:
	std::unique_ptr<IShader> m_opaqueShader;
	std::unique_ptr<IShader> m_transparentShader;
	std::unique_ptr<FullscreenQuadShader> m_resolveShader;

	gl::StaticShaderStorageBuffer m_storageBuffer;
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

	const size_t m_samplesPerPixel;
};
