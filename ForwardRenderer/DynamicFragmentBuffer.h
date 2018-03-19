#pragma once
#include "Graphics/IRenderer.h"
#include "Framework/IWindowReceiver.h"
#include "Graphics/GpuTimer.h"
#include "Dependencies/gl/buffer.hpp"
#include "Graphics/Program.h"

class DynamicFragmentBufferRenderer : public IRenderer, public IWindowReceiver
{
public:
	DynamicFragmentBufferRenderer();
	void render(const IModel* model, IShader* shader, const ICamera* camera) override;

	void onSizeChange(int width, int height) override;
private:
	std::unique_ptr<IShader> m_shaderCountFragments;
	gl::DynamicShaderStorageBuffer m_fragmentStorage;
	std::vector<gl::StaticTextureShaderStorageBuffer> m_auxBuffer;
	Program m_scanShader;
	GLsizei m_curScanSize = 0;

	enum Timer
	{
		T_CLEAR,
		T_OPAQUE,
		T_COUNT_FRAGMENTS,
		T_SCAN,
		T_RESIZE,
		T_SORT,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;
};
