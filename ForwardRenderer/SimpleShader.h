#pragma once
#include "Graphics/IShader.h"
#include <memory>
#include "Dependencies/gl/buffer.hpp"
#include "Graphics/SamplerCache.h"
#include "Window.h"
#include "Graphics/HotReloadShader.h"

class SimpleShader : public IShader
{
	struct UniformData
	{
		glm::mat4 model;
		glm::mat4 viewProjection;
		glm::vec3 cameraPosition;
		glm::uint32_t screenWidth = 0;
	};
public:
	/**
	 * \brief 
	 * \param program compiled and linked shader program
	 */
	SimpleShader(std::shared_ptr<HotReloadShader::WatchedProgram> program)
		:
	m_program(std::move(program)),
	m_transformBuffer(sizeof(UniformData))
	{
		assert(m_program);

		m_uniformData.model = glm::mat4();
		m_uniformData.viewProjection = glm::mat4();
		m_uniformData.cameraPosition = glm::vec4();

		m_transformBuffer.update(&m_uniformData);
	}
	~SimpleShader() override = default;

	void bind() const override
	{
		m_program->getProgram().bind();
		m_transformBuffer.bind(0);
	}

	void applyCamera(const ICamera& camera) override
	{
		if (camera.getPosition() != m_uniformData.cameraPosition
			|| camera.getProjection() != m_uniformData.viewProjection
			|| Window::getWidth() != m_uniformData.screenWidth)
		{
			m_uniformData.cameraPosition = camera.getPosition();
			m_uniformData.viewProjection = camera.getProjection();
			m_uniformData.screenWidth = Window::getWidth();

			m_transformBuffer.update(&m_uniformData);
		}
	}

	void setModelTransform(const glm::mat4& transform) override
	{
		if(transform != m_uniformData.model)
		{
			m_uniformData.model = transform;
			m_transformBuffer.update(&m_uniformData);
		}
	}
private:
	std::shared_ptr<HotReloadShader::WatchedProgram> m_program;

	gl::DynamicUniformBuffer m_transformBuffer;
	UniformData m_uniformData;
};
