#pragma once
#include <glad/glad.h>
#include "Texture2D.h"

class Framebuffer
{
public:
	Framebuffer()
	{
		glGenFramebuffers(1, &m_id);
	}
	~Framebuffer()
	{
		glDeleteFramebuffers(1, &m_id);
	}

	void attachColorTarget(const Texture2D& texture, size_t index)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GLenum(GL_COLOR_ATTACHMENT0 + index), texture.getId(), 0);

		m_drawBuffers.push_back(GLenum(GL_COLOR_ATTACHMENT0 + index));
	}

	void bind()
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
	}

	void attachDepthTarget(const Texture2D& depth)
	{
		assert(!m_hasDepth);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth.getId(), 0);

		m_hasDepth = true;
	}

	static void unbind()
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	void validate()
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
		GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
			throw std::runtime_error("invalid framebuffer");

		// Enable all the attached targets from the list.
		glDrawBuffers(static_cast<GLsizei>(m_drawBuffers.size()), m_drawBuffers.data());
	}

private:
	GLuint m_id = 0;
	std::vector<GLenum> m_drawBuffers;
	bool m_hasDepth = false;
};
