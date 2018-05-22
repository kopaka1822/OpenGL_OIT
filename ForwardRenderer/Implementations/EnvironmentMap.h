#pragma once
#include "../Dependencies/gl/texture.hpp"
#include "../Graphics/IShader.h"
#include "../Dependencies/gl/framebuffer.hpp"
#include "../Graphics/IModel.h"
#include "EnvmapCamera.h"
#include "../Graphics/ITransforms.h"


class EnvironmentMap
{
public:
	EnvironmentMap(int resolution)
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

		gl::Framebuffer::unbind();
	}

	void render(const IModel& model, IShader& shader, const ICamera& cam, ITransforms& transforms)
	{
		EnvmapCamera envcam = EnvmapCamera(cam.getPosition());

		// draw from all directions
		model.prepareDrawing(shader);
		for(auto i = 0; i < m_fbos.size(); ++i)
		{
			envcam.rotateForFace(i);
			transforms.update(envcam);
			transforms.upload();
			transforms.bind();

			m_fbos[i].bind();

			for(auto& shape : model.getShapes())
			{
				if (!shape->isTransparent()) shape->draw(&shader);
			}
		}

		gl::Framebuffer::unbind();
		transforms.update(cam);
		transforms.upload();
	}

	void bind(int slot) const
	{
		m_cubeMap.bind(slot);
	}

private:

	gl::TextureCubeMap m_cubeMap;
	gl::Renderbuffer m_depth;
	std::array<gl::Framebuffer, 6> m_fbos;
};
