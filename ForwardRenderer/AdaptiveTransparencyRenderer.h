#pragma once
#include "Graphics/IRenderer.h"
#include "Graphics/IShader.h"
#include "Graphics/Texture3D.h"
#include "Framework/IWindowReceiver.h"
#include "Graphics/ShaderStorageBuffer.h"

class AdaptiveTransparencyRenderer : public IRenderer, public IWindowReceiver
{
public:
	
	AdaptiveTransparencyRenderer();

	void render(const IModel* model, IShader* shader, const ICamera* camera) override;
	void onSizeChange(int width, int height) override;
private:
	std::unique_ptr<IShader> m_shaderBuildVisz;
	std::unique_ptr<IShader> m_shaderApplyVisz;
	std::unique_ptr<Texture3D> m_visibilityFunc;
	std::vector<glm::vec2> m_emptyVisibilityFuncData;
	std::unique_ptr<ShaderStorageBuffer> m_mutexBuffer;
	std::unique_ptr<Texture2D> m_mutexTexture;
	std::vector<GLuint> m_mutexData;
};
