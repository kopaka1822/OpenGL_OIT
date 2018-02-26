#pragma once
#include "Graphics/IRenderer.h"
#include "Graphics/GpuTimer.h"

class SimpleForwardRenderer :
	public IRenderer
{
public:
	SimpleForwardRenderer();
	~SimpleForwardRenderer();

	void render(const IModel* model, IShader* shader, const ICamera* camera) override;

private:
	GpuTimer m_timer;
};

