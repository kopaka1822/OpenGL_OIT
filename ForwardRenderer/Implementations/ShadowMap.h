#pragma once
#include "../Graphics/CachedTexture2D.h"
#include "../Dependencies/gl/framebuffer.hpp"
#include "../Graphics/IModel.h"

class ShadowMap
{
public:
	ShadowMap(int resolution) 
		:
	m_resolution(resolution)
	{
		// create framebuffer
		m_depthTex = gl::Texture2D(gl::InternalFormat::DEPTH_COMPONENT32F, resolution, resolution);

		m_framebuffer.attachDepth(m_depthTex);
		m_framebuffer.validate();
	}
	
	void Update(IModel& model, const ParamSet& light)
	{
		m_framebuffer.bind();
		glViewport(0, 0, m_resolution, m_resolution);
		glClear(GL_DEPTH_BUFFER_BIT);

		if(light.has("position"))
		{
			// point light
			throw std::runtime_error("not implemented");
		}
		else if(light.has("direction"))
		{
			// directional light

			//model.prepareDrawing()
			for (auto& shape : model.getShapes())
			{
				if (!shape->isTransparent())
				{
					//shape->draw();
				}
			}

		}
		else throw std::runtime_error("cannot make a shadow map for unknown light type");

		gl::Framebuffer::unbind();

		// TODO restore viewport
	}

private:
	

private:
	gl::Texture2D m_depthTex;
	gl::Framebuffer m_framebuffer;
	const int m_resolution;
};
