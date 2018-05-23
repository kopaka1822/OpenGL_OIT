#pragma once
#include "id.h"
#include "texture.hpp"
#include "renderbuffer.hpp"
#include <vector>

namespace gl
{
	class Framebuffer
	{
		Framebuffer(bool)
		{}
	public:
		static Framebuffer empty()
		{
			return Framebuffer(false);
		}
		Framebuffer()
		{
			glGenFramebuffers(1, &m_id);
		}
		~Framebuffer()
		{
			glDeleteFramebuffers(1, &m_id);
		}

		Framebuffer(const Framebuffer&) = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
		Framebuffer(Framebuffer&&) = default;
		Framebuffer& operator=(Framebuffer&&) = default;

		template<GLenum TType, GLsizei TComponents>
		void attachDepth(const Texture<TType, TComponents>& target, GLuint mipLevel = 0)
		{
			bind();
			glFramebufferTexture(GL_DRAW_FRAMEBUFFER, isStencilFormat(target.format()) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, target.getId(), mipLevel);
		}
		template<GLenum TType, GLsizei TComponents>
		void attachDepth(const Texture<TType, TComponents>& target, GLuint mipLevel, GLuint layer)
		{
			bind();
			glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, isStencilFormat(target.format()) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, target.getId(), mipLevel, layer);
		}
		void attachDepth(const Renderbuffer& target)
		{
			bind();
			glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, isStencilFormat(target.format()) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.getId());
		}
		
		template<GLenum TType, GLsizei TComponents>
		void attachColor(GLuint index, const Texture<TType, TComponents>& target, GLuint mipLevel = 0)
		{
			bind();
			glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target.getId(), mipLevel);

			m_attachments.push_back(GL_COLOR_ATTACHMENT0 + index);
		}

		template<GLenum TType, GLsizei TComponents>
		void attachColor(GLuint index, const Texture<TType, TComponents>& target, GLuint mipLevel, GLuint layer)
		{
			bind();
			glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target.getId(), mipLevel, layer);

			m_attachments.push_back(GL_COLOR_ATTACHMENT0 + index);
		}

		void validate()
		{
			bind();
			const auto status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
			if(status != GL_FRAMEBUFFER_COMPLETE)
				throw std::runtime_error("invalid framebuffer");

			// enable buffers
			glDrawBuffers(GLsizei(m_attachments.size()), m_attachments.data());
		}

		void bind() const
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
		}
		static void unbind()
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
	private:
		unique<GLuint> m_id;
		std::vector<GLenum> m_attachments;
	};
}
