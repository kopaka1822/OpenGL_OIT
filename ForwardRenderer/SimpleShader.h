#pragma once
#include "Graphics/IShader.h"
#include "Graphics/Program.h"
#include <memory>
#include "Graphics/IMaterial.h"
#include "Dependencies/gl/buffer.hpp"
#include "Graphics/SamplerCache.h"

class SimpleShader : public IShader
{
	struct UniformData
	{
		glm::mat4 model;
		glm::mat4 viewProjection;
		glm::vec3 cameraPosition;
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
	static Program getLinkedDefaultProgram()
	{
		auto vertex = Shader::loadFromFile(GL_VERTEX_SHADER, "Shader/DefaultShader.vs");
		auto fragment = Shader::loadFromFile(GL_FRAGMENT_SHADER, "Shader/DefaultShader.fs");
		auto geometry = Shader::loadFromFile(GL_GEOMETRY_SHADER, "Shader/DefaultShader.gs");

		Program p;
		p.attach(vertex).attach(fragment).attach(geometry).link();
		return p;
	}
	/**
	 * \brief 
	 * \param program compiled and linked shader program
	 */
	SimpleShader(Program program)
		:
	m_program(std::move(program)),
	m_transformBuffer(sizeof(UniformData)),
	m_materialBuffer(sizeof(MaterialData)),
	m_sampler(SamplerCache::getSampler(gl::MinFilter::LINEAR, gl::MagFilter::LINEAR, gl::MipFilter::LINEAR))
	{
		m_uniformData.model = glm::mat4();
		m_uniformData.viewProjection = glm::mat4();
		m_uniformData.cameraPosition = glm::vec4();

		m_transformBuffer.update(&m_uniformData);

		m_defaultTex = CachedTexture2D::loadConstant(glm::vec4(1.0f));
	}
	~SimpleShader() override = default;

	void bind() const override
	{
		m_program.bind();
		m_transformBuffer.bind(0);
	}

	void applyCamera(const ICamera& camera) override
	{
		if (camera.getPosition() != m_uniformData.cameraPosition
			|| camera.getProjection() != m_uniformData.viewProjection)
		{
			m_uniformData.cameraPosition = camera.getPosition();
			m_uniformData.viewProjection = camera.getProjection();

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

	void setMaterial(const IMaterial& material) override
	{
		// set texture bindings
		m_sampler.bind(DIFFUSE_BINDING);
		m_sampler.bind(DISSOLVE_BINDING);
		m_sampler.bind(AMBIENT_BINDING);
		m_sampler.bind(SPECULAR_BINDING);

		auto tex = material.getTexture("diffuse");
		if (tex) tex->bind(DIFFUSE_BINDING);
		else m_defaultTex->bind(DIFFUSE_BINDING);

		tex = material.getTexture("dissolve");
		if (tex) tex->bind(DISSOLVE_BINDING);
		else m_defaultTex->bind(DISSOLVE_BINDING);

		tex = material.getTexture("ambient");
		if (tex) tex->bind(AMBIENT_BINDING);
		else m_defaultTex->bind(AMBIENT_BINDING);

		tex = material.getTexture("specular");
		if (tex) tex->bind(SPECULAR_BINDING);
		else m_defaultTex->bind(SPECULAR_BINDING);

		// set material data
		auto ambient = material.getAttribute("ambient");
		if (ambient)
			m_materialData.m_ambient = glm::vec3(ambient->r, ambient->g, ambient->b);
		else
			m_materialData.m_ambient = glm::vec3(0.0f);

		auto diffuse = material.getAttribute("diffuse");
		if (diffuse)
			m_materialData.m_diffuse = *diffuse;
		else
			m_materialData.m_diffuse = glm::vec4(0.5f);

		auto specular = material.getAttribute("specular");
		if (specular)
			m_materialData.m_specular = *specular;
		else
			m_materialData.m_specular = glm::vec4(0.0f);

		auto dissolve = material.getAttribute("dissolve");
		if (dissolve)
			m_materialData.m_dissolve = dissolve->r;
		else
			m_materialData.m_dissolve = 1.0f;

		// upload the data
		m_materialBuffer.update(&m_materialData);
		m_materialBuffer.bind(1);
	}
private:
	Program m_program;
	gl::DynamicUniformBuffer m_transformBuffer;
	gl::DynamicUniformBuffer m_materialBuffer;
	UniformData m_uniformData;
	MaterialData m_materialData;
	std::shared_ptr<CachedTexture2D> m_defaultTex;
	gl::Sampler& m_sampler;
};
