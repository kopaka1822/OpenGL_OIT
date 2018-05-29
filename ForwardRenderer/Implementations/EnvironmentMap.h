#pragma once
#include "../Dependencies/gl/texture.hpp"
#include "../Graphics/IShader.h"
#include "../Dependencies/gl/framebuffer.hpp"
#include "../Graphics/IModel.h"
#include "EnvmapCamera.h"
#include "../Graphics/ITransforms.h"
#include "../Graphics/SamplerCache.h"
#include "../Graphics/IEnvironmentMap.h"
#include <glad/glad.h>


class EnvironmentMap : public IEnvironmentMap
{
public:
	EnvironmentMap(int resolution)
		:
	m_sampler(SamplerCache::getSampler(gl::MinFilter::LINEAR, gl::MagFilter::LINEAR, gl::MipFilter::LINEAR)),
	m_resolution(resolution)
	{
		m_cubeMap = gl::TextureCubeMap(gl::InternalFormat::RGBA8, gl::computeMaxMipMapLevels(resolution));
		m_cubeMap.resize(resolution, resolution);

		m_depth = gl::Renderbuffer(gl::InternalFormat::DEPTH_COMPONENT32F, resolution, resolution);

		for(auto i = 0; i < m_fbos.size(); ++i)
		{
			m_fbos[i].attachColor(0, m_cubeMap, 0, i);
			m_fbos[i].attachDepth(m_depth);
			m_fbos[i].validate();
		}

		m_cubeMap.unbind(8);
		gl::Framebuffer::unbind();
	}

	void render(const IModel& model, IShader& shader, const ICamera& cam, ITransforms& transforms, const glm::vec3& center) override
	{
		EnvmapCamera envcam = EnvmapCamera(center);

		m_cubeMap.unbind(8);

		transforms.setModelTransform(glm::mat4(1.0f));
		// draw from all directions
		model.prepareDrawing(shader);
		for(auto i = 0; i < m_fbos.size(); ++i)
		{
			m_fbos[i].bind();
			glViewport(0, 0, m_resolution, m_resolution);
			// clear buffer
			glEnable(GL_DEPTH_TEST);
			//IRenderer::setClearColor();
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			envcam.rotateForFace(i);
			transforms.update(envcam);
			transforms.upload();
			transforms.bind();

			for(auto& shape : model.getShapes())
			{
				if (!shape->isTransparent()) shape->draw(&shader);
			}
		}

		gl::Framebuffer::unbind();
		m_cubeMap.generateMipmaps();

		transforms.update(cam);
		transforms.upload();
	}

	void bind() const override
	{
		m_sampler.bind(8);
		m_cubeMap.bind(8);
	}

private:
	gl::Sampler& m_sampler;
	gl::TextureCubeMap m_cubeMap;
	gl::Renderbuffer m_depth;
	std::array<gl::Framebuffer, 6> m_fbos;
	const int m_resolution;
};
