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

	struct MaterialData
	{
		glm::vec3 m_ambient;
		float m_dissolve;
		glm::vec4 m_diffuse;
		glm::vec4 m_specular;
	};

	enum MaterialTextureBinding
	{
		AMBIENT_BINDING = 0,
		DISSOLVE_BINDING = 1,
		DIFFUSE_BINDING = 2,
		SPECULAR_BINDING = 3,
	};
public:
	/**
	 * \brief 
	 * \param program compiled and linked shader program
	 */
	SimpleShader(std::shared_ptr<HotReloadShader::WatchedProgram> program)
		:
	m_program(std::move(program)),
	m_transformBuffer(sizeof(UniformData)),
	m_materialBuffer(sizeof(MaterialData)),
	m_sampler(SamplerCache::getSampler(gl::MinFilter::LINEAR, gl::MagFilter::LINEAR, gl::MipFilter::LINEAR))
	{
		assert(m_program);

		m_uniformData.model = glm::mat4();
		m_uniformData.viewProjection = glm::mat4();
		m_uniformData.cameraPosition = glm::vec4();

		m_transformBuffer.update(&m_uniformData);

		m_defaultTex = CachedTexture2D::loadConstant(glm::vec4(1.0f));
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

	void setMaterial(const ParamSet& material) override
	{
		// set texture bindings
		m_sampler.bind(DIFFUSE_BINDING);
		m_sampler.bind(DISSOLVE_BINDING);
		m_sampler.bind(AMBIENT_BINDING);
		m_sampler.bind(SPECULAR_BINDING);

		material.getTexture("diffuse", m_defaultTex)->bind(DIFFUSE_BINDING);
		material.getTexture("dissolve", m_defaultTex)->bind(DISSOLVE_BINDING);
		material.getTexture("ambient", m_defaultTex)->bind(AMBIENT_BINDING);
		material.getTexture("specular", m_defaultTex)->bind(SPECULAR_BINDING);

		// set material data
		m_materialData.m_ambient = material.get("ambient", glm::vec3(0.0f));
		m_materialData.m_diffuse = material.get("diffuse", glm::vec4(0.5f));
		m_materialData.m_specular = material.get("specular", glm::vec4(0.0f));
		m_materialData.m_dissolve = material.get("dissolve", 1.0f);

		// upload the data
		m_materialBuffer.update(&m_materialData);
		m_materialBuffer.bind(1);
	}
private:
	std::shared_ptr<HotReloadShader::WatchedProgram> m_program;

	gl::DynamicUniformBuffer m_transformBuffer;
	gl::DynamicUniformBuffer m_materialBuffer;
	UniformData m_uniformData;
	MaterialData m_materialData;
	std::shared_ptr<CachedTexture2D> m_defaultTex;
	gl::Sampler& m_sampler;
};
