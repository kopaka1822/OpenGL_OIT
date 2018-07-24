#pragma once

#include "../Graphics/IRenderer.h"
#include "../Graphics/GpuTimer.h"
#include "../Implementations/EnvironmentMap.h"

class DebugRenderer :
	public IRenderer
{
public:
	DebugRenderer();
	~DebugRenderer();

	void render(const RenderArgs& args) override;

	void init() override;
private:
	std::unique_ptr<IShader> m_normalShader;
	std::unique_ptr<IShader> m_texcoordShader;
	std::unique_ptr<IShader> m_meshShader;
	IShader* m_activeShader = nullptr;
};

