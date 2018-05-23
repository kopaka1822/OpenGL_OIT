#pragma once
#include "../Graphics/IRenderer.h"
#include "../Graphics/GpuTimer.h"
#include "../Implementations/EnvironmentMap.h"

class SimpleForwardRenderer :
	public IRenderer
{
public:
	SimpleForwardRenderer();
	~SimpleForwardRenderer();

	void render(const RenderArgs& args) override;

private:
	std::unique_ptr<IShader> m_defaultShader;

	enum Timer
	{
		T_OPAQUE,
		T_TRANSPARENT,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;

	EnvironmentMap m_envmap;
};

