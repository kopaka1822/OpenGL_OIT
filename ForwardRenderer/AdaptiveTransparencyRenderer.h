#pragma once
#include "Graphics/IRenderer.h"
#include "Graphics/IShader.h"
#include "Framework/IWindowReceiver.h"
#include "FullscreenQuadShader.h"
#include <array>
#include "Graphics/GpuTimer.h"
#include "Dependencies/gl/texture.hpp"

class AdaptiveTransparencyRenderer : public IRenderer, public IWindowReceiver
{
public:
	
	AdaptiveTransparencyRenderer();

	void render(const IModel* model, IShader* shader, const ICamera* camera) override;
	void onSizeChange(int width, int height) override;
private:
	std::unique_ptr<IShader> m_shaderBuildVisz;
	std::unique_ptr<IShader> m_shaderApplyVisz;
	gl::Texture3D m_visibilityFunc;
	gl::Texture2D m_mutexTexture;
	std::unique_ptr<FullscreenQuadShader> m_shaderAdjustBackground;
	const glm::vec2 m_visibilityClearColor;

	enum Timer
	{
		T_CLEAR,
		T_OPAQUE,
		T_BUILD_VIS,
		T_USE_VIS,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;
};
