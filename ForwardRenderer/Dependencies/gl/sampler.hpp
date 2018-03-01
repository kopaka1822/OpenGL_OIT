#pragma once
#include "id.h"
#include "../opengl.h"
#include <array>

namespace gl
{
	enum class BorderHandling {
		REPEAT = GL_REPEAT,
		CLAMP = GL_CLAMP_TO_EDGE,
		BORDER = GL_CLAMP_TO_BORDER,
		MIRROR = GL_MIRRORED_REPEAT
	};

	enum struct MipFilter {
		NONE,
		NEAREST = GL_NEAREST,
		LINEAR = GL_LINEAR
	};

	enum struct MagFilter {
		NEAREST = GL_NEAREST,
		LINEAR = GL_LINEAR
	};

	enum struct MinFilter {
		NEAREST = GL_NEAREST,
		LINEAR = GL_LINEAR
	};

	enum struct DepthCompareFunc {
		LESS = GL_LESS,
		LESS_EQUAL = GL_LEQUAL,
		GREATER = GL_GREATER,
		GREATER_EQUAL = GL_GEQUAL,
		EQUAL = GL_EQUAL,
		NOT_EQUAL = GL_NOTEQUAL,
		NEVER = GL_NEVER,
		ALWAYS = GL_ALWAYS,
		DISABLE
	};

	class Sampler
	{
	public:
		Sampler() = default;
		Sampler(MinFilter minFilter, MagFilter magFilter, MipFilter mipFilter = MipFilter::NONE, 
			BorderHandling border = BorderHandling::REPEAT, DepthCompareFunc depthFunc = DepthCompareFunc::DISABLE, std::array<float, 4> borderColor = {0.0f, 0.0f, 0.0f, 0.0f})
		{
			glGenSamplers(1, &m_id);

			// border
			glSamplerParameteri(m_id, GL_TEXTURE_WRAP_R, static_cast<GLenum>(border));
			glSamplerParameteri(m_id, GL_TEXTURE_WRAP_S, static_cast<GLenum>(border));
			glSamplerParameteri(m_id, GL_TEXTURE_WRAP_T, static_cast<GLenum>(border));

			// filter
			if (mipFilter == MipFilter::NONE)
				glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(minFilter));
			else
				glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, minFilter == MinFilter::NEAREST ?
				(mipFilter == MipFilter::NEAREST ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR) :
					(mipFilter == MipFilter::NEAREST ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR)
				);
			glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(magFilter));

			// depth compare (for shadow mapping)
			if(depthFunc != DepthCompareFunc::DISABLE)
			{
				glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_FUNC, static_cast<GLint>(depthFunc));
			}

			// border color
			glSamplerParameterfv(m_id, GL_TEXTURE_BORDER_COLOR, borderColor.data());
		}
		~Sampler()
		{
			glDeleteSamplers(1, &m_id);
		}
		Sampler(const Sampler&) = delete;
		Sampler& operator=(const Sampler&) = delete;
		Sampler(Sampler&&) = default;
		Sampler& operator=(Sampler&&) = default;
	private:
		unique<GLuint> m_id;
	};
}
