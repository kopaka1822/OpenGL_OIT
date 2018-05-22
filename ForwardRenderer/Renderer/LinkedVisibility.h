#pragma once
#include "../Graphics/IRenderer.h"
#include "../Framework/IWindowReceiver.h"
#include "../Implementations/FullscreenQuadShader.h"
#include "../Graphics/GpuTimer.h"
#include <array>
#include "../Dependencies/gl/buffer.hpp"
#include "../Dependencies/gl/texture.hpp"

class LinkedVisibility : public IRenderer, public IWindowReceiver
{
public:
	LinkedVisibility();

	void render(const IModel* model, const ICamera* camera, ILights* lights) override;
	void onSizeChange(int width, int height) override;

private:
	std::unique_ptr<IShader> m_defaultShader;
	std::unique_ptr<IShader> m_shaderBuildVisz;
	std::unique_ptr<IShader> m_shaderApplyVisz;
	std::unique_ptr<FullscreenQuadShader> m_shaderAdjustBackground;
	gl::DynamicShaderStorageBuffer m_buffer;
	gl::Texture2D m_mutexTexture;
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
