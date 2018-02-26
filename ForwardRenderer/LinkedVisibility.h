#pragma once
#include "Graphics/IRenderer.h"
#include "Framework/IWindowReceiver.h"
#include "Graphics/UniformBuffer.h"
#include "Graphics/ShaderStorageBuffer.h"
#include "FullscreenQuadShader.h"
#include "Framework/AtomicCounterBuffer.h"
#include "Graphics/GpuTimer.h"

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
	std::unique_ptr<ShaderStorageBuffer> m_buffer;
	std::unique_ptr<Texture2D> m_mutexTexture;
	std::unique_ptr<AtomicCounterBuffer> m_counter;
	GpuTimer m_timer;
};
