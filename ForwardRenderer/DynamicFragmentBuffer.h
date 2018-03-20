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
	void performScan();

private:
	std::unique_ptr<IShader> m_shaderCountFragments;
	std::unique_ptr<IShader> m_shaderStoreFragments;
	std::unique_ptr<IShader> m_shaderSortFragments;

	gl::DynamicShaderStorageBuffer m_fragmentStorage;
	gl::StaticTextureShaderStorageBuffer m_countingBuffer;
	std::vector<gl::StaticTextureShaderStorageBuffer> m_auxBuffer;
	gl::StaticClientShaderStorageBuffer m_scanStageBuffer;

	Program m_scanShader;
	Program m_pushScanShader;
	GLsizei m_curScanSize = 0;
	GLsizei m_curLastIndex = 0;

	enum Timer
	{
		T_CLEAR,
		T_OPAQUE,
		T_COUNT_FRAGMENTS,
		T_SCAN,
		T_RESIZE,
		T_STORE_FRAGMENTS,
		T_SORT,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;
};
