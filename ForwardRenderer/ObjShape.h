#pragma once
#include "Graphics/IShape.h"
#include <vector>
#include "ObjModel.h"
#include "Graphics/IShader.h"

class ObjShape : public IShape
{
public:
	ObjShape(gl::StaticArrayBuffer& buffer, std::unique_ptr<IMaterials::Instance> material)
		:
	m_material(move(material)),
	m_elements(std::move(buffer))
	{
		const auto& mat = m_material->getMaterial();
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
		if (shader)
		{
			m_material->bind();
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
	std::unique_ptr<IMaterials::Instance> m_material;
	gl::StaticArrayBuffer m_elements;
	bool m_isTransparent = false;
};
