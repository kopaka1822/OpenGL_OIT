#pragma once
#include "Graphics/IRenderer.h"
#include "Graphics/GpuTimer.h"

class SimpleForwardRenderer :
	public IRenderer
{
public:
	SimpleForwardRenderer();
	~SimpleForwardRenderer();

	void render(const IModel* model, const ICamera* camera, ILights* lights) override;

private:
	std::unique_ptr<IShader> m_defaultShader;

	enum Timer
	{
		T_OPAQUE,
		T_TRANSPARENT,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;
};

