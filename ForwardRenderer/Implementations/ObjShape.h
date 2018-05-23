#pragma once
#include "../Graphics/IShape.h"
#include <vector>
#include "ObjModel.h"
#include "../Graphics/IShader.h"
#include "../Graphics/IRenderer.h"

class ObjShape : public IShape
{
public:
	ObjShape(gl::StaticArrayBuffer& buffer, ObjModel& model, int materialId)
		:
	m_model(model),
	m_materialIndex(materialId),
	m_elements(std::move(buffer))
	{
		const auto& mat = m_model.getMaterial().getMaterial(materialId);
		// transparent
		auto dissolve = mat.get<float>("dissolve");
		if (dissolve && *dissolve < 1.0f)
			m_isTransparent = true;
		if (mat.getTexture("dissolve"))
			m_isTransparent = true;
		auto diffuseTex = mat.getTexture("diffuse");
		if (diffuseTex && diffuseTex->isTransparent())
			m_isTransparent = true;
	}

	void draw(IShader* shader) override
	{
		if (IRenderer::s_filterMaterial != -1 && IRenderer::s_filterMaterial > m_materialIndex) return;

		if (shader)
		{
			m_model.getMaterial().bind(m_materialIndex);
			shader->bind();
		}

		m_elements.bind(0);
		glDrawArrays(GL_TRIANGLES, 0, m_elements.getNumElements());
	}

	bool isTransparent() const override
	{
		return m_isTransparent;
	}
private:
	ObjModel& m_model;
	const int m_materialIndex;
	gl::StaticArrayBuffer m_elements;
	bool m_isTransparent = false;
};
