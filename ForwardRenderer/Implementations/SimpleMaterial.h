#pragma once
#include "../Graphics/IMaterials.h"
#include "../Dependencies/gl/buffer.hpp"
#include "../Graphics/SamplerCache.h"
#include "../Dependencies/gl/constants.h"

class SimpleMaterial : public IMaterials
{
	struct MaterialData
	{
		glm::vec3 ambient;
		float dissolve;

		glm::vec3 diffuse;
		int illum;
		
		// specular + shininess
		glm::vec4 specular;
	};

	enum MaterialTextureBinding
	{
		AMBIENT_BINDING = 0,
		DISSOLVE_BINDING = 1,
		DIFFUSE_BINDING = 2,
		SPECULAR_BINDING = 3,
	};

public:
	class Instance : public IMaterials::Instance
	{
	public:

		Instance(const SimpleMaterial* materials, int id)
			: m_parent(materials),
			  materialId(id)
		{
		}
		void bind() const override
		{
			m_parent->bind(materialId);
		}
		const ParamSet& getMaterial() const override
		{
			return m_parent->getMaterial(materialId);
		}
	private:
		const SimpleMaterial* m_parent;
		int materialId;
	};

public:
	SimpleMaterial()
		:
	m_sampler(SamplerCache::getSampler(gl::MinFilter::LINEAR, gl::MagFilter::LINEAR, gl::MipFilter::LINEAR)),
	m_defaultTex(CachedTexture2D::loadConstant(glm::vec4(1.0f)))
	{}

	void addMaterial(ParamSet material) override
	{
		m_materials.push_back(std::move(material));
	}


	void reserve(size_t count) override
	{
		m_materials.reserve(count);
	}

	void upload() override
	{
		
		auto alignment = size_t(gl::get<gl::GetParam::UNIFORM_BUFFER_OFFSET_ALIGNMENT>::value());

		auto padding = (alignment - (sizeof(MaterialData) % alignment) % alignment);
		auto paddedLength = sizeof(MaterialData) + padding;

		auto data = std::make_unique<uint8_t[]>(paddedLength * m_materials.size());
		
		auto cur = data.get();
		for(auto i = m_materials.begin(), end = m_materials.end(); i != end; ++i)
		{
			MaterialData* d = reinterpret_cast<MaterialData*>(cur);
			d->ambient = i->get("ambient", glm::vec3(0.0f));
			d->diffuse = i->get("diffuse", glm::vec4(0.5f));
			d->specular = i->get("specular", glm::vec4(0.0f));
			d->dissolve = i->get("dissolve", 1.0f);
			d->illum = int(i->get("illum", 1.0f));

			cur += paddedLength;
		}

		m_uniform = gl::StaticUniformBuffer(GLsizei(paddedLength), GLsizei(m_materials.size()), data.get());
	}

	void bind(int materialId) const override
	{
		const auto& mat = m_materials.at(materialId);

		// bind textures
		m_sampler.bind(DIFFUSE_BINDING);
		m_sampler.bind(DISSOLVE_BINDING);
		m_sampler.bind(AMBIENT_BINDING);
		m_sampler.bind(SPECULAR_BINDING);

		mat.getTexture("diffuse", m_defaultTex)->bind(DIFFUSE_BINDING);
		mat.getTexture("dissolve", m_defaultTex)->bind(DISSOLVE_BINDING);
		mat.getTexture("ambient", m_defaultTex)->bind(AMBIENT_BINDING);
		mat.getTexture("specular", m_defaultTex)->bind(SPECULAR_BINDING);

		// bind appropriate uniform buffer range
		m_uniform.bind(1, materialId, 1);
	}

	const ParamSet& getMaterial(int materialId) const
	{
		return m_materials.at(materialId);
	}


	std::unique_ptr<IMaterials::Instance> getInstance(int materialId) const override
	{
		return std::make_unique<Instance>(this, materialId);
	}
private:
	std::vector<ParamSet> m_materials;
	gl::StaticUniformBuffer m_uniform;
	gl::Sampler& m_sampler;
	std::shared_ptr<CachedTexture2D> m_defaultTex;
};
