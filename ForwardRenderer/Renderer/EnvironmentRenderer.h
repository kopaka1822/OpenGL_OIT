#pragma once
#include "../Graphics/IRenderer.h"
#include "../Framework/IMouseReceiver.h"
#include "../Implementations/FullscreenQuadShader.h"

class EnvironmentRenderer : public IRenderer, IMouseReceiver
{
public:
	EnvironmentRenderer();

	void render(const RenderArgs& args) override;
	~EnvironmentRenderer() override = default;

	void onMouseMove(double x, double y, double dx, double dy) override;
	void onButtonDown(int button) override;
	void onButtonUp(int button) override;
	void onScroll(double x, double y) override;

private:
	glm::mat4 calcTransform() const;
	float calcFarplane() const;

private:
	float m_pitch = 0.0f;
	float m_roll = 0.0f;
	float m_aperture = 1.0f;
	bool m_mouseDown = false;

	std::unique_ptr<FullscreenQuadShader> m_shader;
};
