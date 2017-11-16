#pragma once
#include "Graphics/IShape.h"
#include <vector>
#include "ObjModel.h"
#include "Graphics/ElementBuffer.h"
#include "Graphics/IShader.h"

class ObjShape : public IShape
{
public:
	ObjShape(const std::vector<int>& indices, const IMaterial& material)
		:
	m_material(material),
	m_elements(indices)
	{
		
	}

	void draw(IShader* shader) override
	{
		if (shader)
		{
			shader->setMaterial(m_material);
			shader->bind();
		}

		m_elements.drawElements();
	}
private:
	const IMaterial& m_material;
	ElementBuffer m_elements;
};
