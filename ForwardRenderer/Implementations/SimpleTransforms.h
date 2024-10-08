#pragma once
#include "../Graphics/ITransforms.h"
#include "../Renderer/DynamicFragmentBuffer.h"

class SimpleTransforms : public ITransforms
{
	struct UniformData
	{
		glm::mat4 model;
		glm::mat4 viewProjection;
		glm::vec3 cameraPosition;
		glm::uint32_t screenWidth = 0;
		float farPlane = 0.0f;
	};
public:
	SimpleTransforms()
		:
	m_uniform(sizeof(UniformData))
	{}

	void upload() override
	{
		if(m_changed)
			m_uniform.update(&m_data);
	}

	void bind() const override
	{
		m_uniform.bind(0);
	}

	void update(const ICamera& camera) override
	{
		m_changed |= m_data.cameraPosition != camera.getPosition();
		m_data.cameraPosition = camera.getPosition();

		m_changed |= m_data.viewProjection != camera.getProjection();
		m_data.viewProjection = camera.getProjection();

		m_changed |= m_data.screenWidth != Window::getWidth();
		m_data.screenWidth = Window::getWidth();

		m_changed |= m_data.farPlane != camera.getFarPlane();
		m_data.farPlane = camera.getFarPlane();
	}
	
	void setModelTransform(const glm::mat4& model) override
	{
		m_changed |= m_data.model != model;
		m_data.model = model;
	}


	const glm::mat4& getModelTransform() override
	{
		return m_data.model;
	}
private:
	UniformData m_data;
	gl::DynamicUniformBuffer m_uniform;
	bool m_changed = true;
};
