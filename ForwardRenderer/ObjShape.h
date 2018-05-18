#pragma once
#include "Graphics/IShape.h"
#include <vector>
#include "ObjModel.h"
#include "Graphics/IShader.h"

class ObjShape : public IShape
{
public:
	ObjShape(gl::StaticArrayBuffer& buffer, const ParamSet& material)
		:
	m_material(material),
	m_elements(std::move(buffer))
	{
		// transparent
		auto dissolve = material.get<float>("dissolve");
		if (dissolve && *dissolve < 1.0f)
			m_isTransparent = true;
		if (material.getTexture("dissolve"))
			m_isTransparent = true;
		auto diffuseTex = material.getTexture("diffuse");
		if (diffuseTex && diffuseTex->isTransparent())
			m_isTransparent = true;
	}

	void draw(IShader* shader) override
	{
		if (shader)
		{
			shader->setMaterial(m_material);
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
	const ParamSet& m_material;
	gl::StaticArrayBuffer m_elements;
	bool m_isTransparent = false;
};
