#pragma once
#include "../Graphics/IShadows.h"
#include "../Dependencies/gl/framebuffer.hpp"
#include "../Graphics/SamplerCache.h"
#include "../Graphics/IModel.h"
#include "../Graphics/ITransforms.h"
#include "../Graphics/HotReloadShader.h"
#include "SimpleShader.h"
#include "EnvmapCamera.h"

class ShadowMaps : public IShadows
{
public:
	ShadowMaps(int resolution)
		:
	m_resolution(resolution),
	m_shadowSampler(SamplerCache::getSampler(gl::MinFilter::LINEAR, gl::MagFilter::LINEAR, gl::MipFilter::NONE, gl::BorderHandling::CLAMP, gl::DepthCompareFunc::GREATER_EQUAL)),
	m_debugSampler(SamplerCache::getSampler(gl::MinFilter::LINEAR, gl::MagFilter::LINEAR, gl::MipFilter::NONE, gl::BorderHandling::CLAMP))
	{
		// unbind the framebuffer
		gl::Framebuffer::unbind();

		auto vert = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/ShadowMap.vs");
		auto frag = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/ShadowMap.fs");

		m_shader = std::make_unique<SimpleShader>(HotReloadShader::loadProgram({ vert, frag }));
	}

	void update(
		const std::vector<PointLight>& pointLights, 
		const std::vector<DirectionalLight>& dirLights,
		const IModel& model, ITransforms& transforms) override
	{
		m_numPointLights = int(pointLights.size());
		if(pointLights.size())
		{
			m_cubeMaps = gl::TextureCubeMapArray(gl::InternalFormat::DEPTH_COMPONENT32F, m_resolution, m_resolution, int(pointLights.size()) * 6, 1);

			for(auto i = 0; i < pointLights.size(); ++i)
			{
				renderPointLight(pointLights[i], i, transforms, model);
			}
		}
		else
		{
			// make dummy texture
			m_cubeMaps = gl::TextureCubeMapArray(gl::InternalFormat::DEPTH_COMPONENT32F, 1, 1, 6, 1);
		}

		if(dirLights.size())
		{
			m_textures = gl::Texture2DArray(gl::InternalFormat::DEPTH_COMPONENT32F, m_resolution, m_resolution, int(dirLights.size()), 1);

			for (auto i = 0; i < dirLights.size(); ++i)
			{
				renderDirLight(dirLights[i], i, transforms, model);
			}
		}
		else
		{
			// make dummy texture
			m_textures = gl::Texture2DArray(gl::InternalFormat::DEPTH_COMPONENT32F, 16, 16, 1, 1);
		}

		// restore framebuffer
		gl::Framebuffer::unbind();
	}

	void bind() const override
	{
		m_shadowSampler.bind(9);
		m_cubeMaps.bind(9);

		m_shadowSampler.bind(10);
		m_textures.bind(10);		
	}

	void bindDebug() const
	{
		m_debugSampler.bind(9);
		m_cubeMaps.bind(9);

		m_debugSampler.bind(10);
		m_textures.bind(10);
	}

	void renderDirLight(const DirectionalLight& light, int index, ITransforms& transforms, const IModel& model)
	{		
		// set and upload light transforms
		transforms.update(light.camera);
		transforms.setModelTransform(glm::mat4(1.0f));
		transforms.upload();

		// attach texture to the frambuffer
		m_framebuffer.attachDepth(m_textures, 0, index);
		m_framebuffer.validate();
		
		transforms.bind();
		render(model);
	}

	void renderPointLight(const PointLight& light, int index, ITransforms& transforms, const IModel& model)
	{
		auto cam = EnvmapCamera(light.position);

		transforms.setModelTransform(glm::mat4(1.0));

		for(int face = 0; face < 6; ++face)
		{
			// attach texture
			m_framebuffer.attachDepth(m_cubeMaps, 0, 6 * index + face);
			m_framebuffer.validate();

			cam.rotateForFace(face);
			transforms.update(cam);
			transforms.upload();
			transforms.bind();
			
			render(model);
		}
	}

	void render(const IModel& model) const
	{
		glViewport(0, 0, m_resolution, m_resolution);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);

		model.prepareDrawing(*m_shader);
		for (const auto& shape : model.getShapes())
		{
			if (!shape->isTransparent())
				shape->draw(m_shader.get());
		}
	}

private:
	const int m_resolution;
	gl::Framebuffer m_framebuffer;

	int m_numPointLights = 0;
	int m_numDirLights = 0;
	gl::TextureCubeMapArray m_cubeMaps;
	gl::Texture2DArray m_textures;

	gl::Sampler& m_shadowSampler;
	gl::Sampler& m_debugSampler;

	glm::vec3 m_bboxCenter;
	std::array<glm::vec3, 8> m_bboxEdges;

	std::unique_ptr<IShader> m_shader;
};
