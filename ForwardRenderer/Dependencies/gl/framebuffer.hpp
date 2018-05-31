#pragma once
#include "id.h"
#include "texture.hpp"
#include "renderbuffer.hpp"
#include <vector>
#include <unordered_set>

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
		
		/**
		 * \brief attaches a texture to the framebuffer. Existing attachments may be overwritten
		 * \param index color attachement index
		 * \param target texture (must be valid)
		 * \param mipLevel texture level
		 */
		template<GLenum TType, GLsizei TComponents>
		void attachColor(GLuint index, const Texture<TType, TComponents>& target, GLuint mipLevel = 0)
		{
			bind();
			assert(target.getId());
			glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target.getId(), mipLevel);

			m_attachments.insert(GL_COLOR_ATTACHMENT0 + index);
		}

		/**
		* \brief attaches a texture to the framebuffer. Existing attachments may be overwritten
		* \param index color attachement index
		* \param target texture (must be valid)
		* \param mipLevel texture level
		* \param layer texture layer
		*/
		template<GLenum TType, GLsizei TComponents>
		void attachColor(GLuint index, const Texture<TType, TComponents>& target, GLuint mipLevel, GLuint layer)
		{
			bind();
			assert(target.getId());
			glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target.getId(), mipLevel, layer);

			m_attachments.insert(GL_COLOR_ATTACHMENT0 + index);
		}

		void detachColor(GLuint index)
		{
			bind();
			glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, 0, 0);

			m_attachments.erase(GL_COLOR_ATTACHMENT0 + index);
		}

		void validate()
		{
			bind();
			const auto status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
			if(status != GL_FRAMEBUFFER_COMPLETE)
				throw std::runtime_error("invalid framebuffer");

			// enable buffers (put set into an array)
			auto vec = std::vector<GLenum>(m_attachments.begin(), m_attachments.end());
			glDrawBuffers(GLsizei(vec.size()), vec.data());
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
		std::unordered_set<GLenum> m_attachments;
	};
}
