#pragma once
#include "Graphics/IRenderer.h"
#include "Framework/IWindowReceiver.h"
#include "FullscreenQuadShader.h"
#include "Graphics/GpuTimer.h"
#include <array>
#include "Dependencies/gl/buffer.hpp"

class LinkedVisibility : public IRenderer, public IWindowReceiver
{
public:
	LinkedVisibility();

	void render(const IModel* model, IShader* shader, const ICamera* camera) override;
	void onSizeChange(int width, int height) override;

private:
	std::unique_ptr<IShader> m_shaderBuildVisz;
	std::unique_ptr<IShader> m_shaderApplyVisz;
	std::unique_ptr<FullscreenQuadShader> m_shaderAdjustBackground;
	gl::DynamicShaderStorageBuffer m_buffer;
	std::unique_ptr<Texture2D> m_mutexTexture;
	gl::DynamicAtomicCounterBuffer m_counter;
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
