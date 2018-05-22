#pragma once
#include "../Graphics/IRenderer.h"
#include "../Framework/IWindowReceiver.h"
#include "../Implementations/FullscreenQuadShader.h"
#include "../Graphics/GpuTimer.h"
#include "../Dependencies/gl/texture.hpp"
#include <array>
#include "../Dependencies/gl/framebuffer.hpp"

class WeightedTransparency : public IRenderer, public IWindowReceiver
{
public:
	WeightedTransparency();
	void render(const IModel* model, const ICamera* camera, ILights* lights, ITransforms* transforms) override;

	void onSizeChange(int width, int height) override;
private:
	gl::Texture2D m_transparentTexture1;
	gl::Texture2D m_transparentTexture2;
	gl::Texture2D m_opaqueTexture;
	gl::Texture2D m_depthTexture;

	gl::Framebuffer m_transparentFramebuffer = gl::Framebuffer::empty();
	gl::Framebuffer m_opaqueFramebuffer = gl::Framebuffer::empty();

	std::unique_ptr<IShader> m_defaultShader;
	std::unique_ptr<FullscreenQuadShader> m_quadShader;
	std::unique_ptr<IShader> m_transShader;

	enum Timer
	{
		T_OPAQUE,
		T_BUILD_VIS,
		T_USE_VIS,
		SIZE
	};
	std::array<GpuTimer, SIZE> m_timer;
};
