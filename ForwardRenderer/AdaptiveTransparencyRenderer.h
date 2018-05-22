#pragma once
#include "Graphics/IRenderer.h"
#include "Graphics/IShader.h"
#include "Framework/IWindowReceiver.h"
#include "FullscreenQuadShader.h"
#include <array>
#include "Graphics/GpuTimer.h"
#include "Dependencies/gl/texture.hpp"
#include "Dependencies/gl/buffer.hpp"

class AdaptiveTransparencyRenderer : public IRenderer, public IWindowReceiver
{
public:
	
	AdaptiveTransparencyRenderer(size_t samplesPerPixel);
	virtual ~AdaptiveTransparencyRenderer();

	void init() override;
	void render(const IModel* model, const ICamera* camera, ILights* lights) override;
	void onSizeChange(int width, int height) override;
private:
	std::unique_ptr<IShader> m_defaultShader;
	std::unique_ptr<IShader> m_shaderBuildVisz;
	std::unique_ptr<IShader> m_shaderApplyVisz;
	gl::Texture3D m_visibilityTex;
	gl::StaticShaderStorageBuffer m_visibilityBuffer;
	gl::TextureBuffer m_visibilityBufferView;

	gl::Texture2D m_mutexTexture;
	std::unique_ptr<FullscreenQuadShader> m_shaderAdjustBackground;
	std::unique_ptr<FullscreenQuadShader> m_shaderClearBackground;
	const glm::vec2 m_visibilityClearColor;

	enum Timer
	{
		T_CLEAR,
		T_OPAQUE,
		T_BUILD_VIS,
		T_DARKEN_BG,
		T_USE_VIS,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;

	const size_t m_samplesPerPixel;
};
