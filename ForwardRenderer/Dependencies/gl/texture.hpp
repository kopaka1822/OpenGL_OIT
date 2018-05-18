#pragma once
#include "id.h"
#include "../opengl.h"
#include "format.h"
#include "sampler.hpp"
#include <array>
#include <numeric>

namespace gl
{
	template<GLenum TType, GLsizei TComponents>
	class Texture
	{
	public:
		Texture() = default;
		template<bool TEnabled = TComponents == 1>
		explicit Texture(InternalFormat internalFormat, std::enable_if_t<TEnabled, GLsizei> width, GLuint mipLevels = 1)
			:
		Texture(internalFormat, mipLevels)
		{
			m_size[0] = width;
			allocateMemory();
		}
		template<bool TEnabled = TComponents == 2>
		explicit Texture(InternalFormat internalFormat, std::enable_if_t<TEnabled, GLsizei> width, GLsizei height, GLuint mipLevels = 1)
			:
		Texture(internalFormat, mipLevels)
		{
			m_size[0] = width;
			m_size[1] = height;
			allocateMemory();
		}
		template<bool TEnabled = TComponents == 3>
		explicit Texture(InternalFormat internalFormat, std::enable_if_t<TEnabled, GLsizei> width, GLsizei height, GLsizei depth, GLuint mipLevels = 1)
			:
		Texture(internalFormat, mipLevels)
		{
			m_size[0] = width;
			m_size[1] = height;
			m_size[2] = depth;
			allocateMemory();
		}

		explicit Texture(InternalFormat internalFormat, GLuint mipLevels = 1)
			:
		m_internalFormat(internalFormat),
		m_mipLevels(mipLevels)
		{
			glGenTextures(1, &m_id);
		}
		~Texture()
		{
			glDeleteTextures(1, &m_id);
		}
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture(Texture&&) = default;
		Texture& operator=(Texture&&) = default;

		void bind(GLuint slot) const
		{
			glActiveTexture(GL_TEXTURE0 + slot);
			bind();
		}

		void bindAsImage(GLuint slot, ImageAccess access, GLuint mipLevel = 0) const
		{
			glBindImageTexture(slot, m_id, mipLevel, GL_TRUE, 0, static_cast<GLenum>(access), static_cast<GLenum>(m_internalFormat.value));
		}

		void bindAsImage(GLuint slot, ImageAccess access, GLuint mipLevel, GLint layer) const
		{
			glBindImageTexture(slot, m_id, mipLevel, GL_FALSE, layer, static_cast<GLenum>(access), static_cast<GLenum>(m_internalFormat.value));
		}

		void update(SetDataFormat format, SetDataType type, const void* data, GLuint mipLevel = 0)
		{
			bind();
			switch (TType)
			{
			case GL_TEXTURE_1D:
				glTexSubImage1D(TType, mipLevel, 0, m_size[0], static_cast<GLenum>(format), static_cast<GLenum>(type), data);
				break;
			case GL_TEXTURE_2D:
				glTexSubImage2D(TType, mipLevel, 0, 0, m_size[0], m_size[1], static_cast<GLenum>(format), static_cast<GLenum>(type), data);
				break;
			case GL_TEXTURE_3D:
			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_CUBE_MAP_ARRAY:
				glTexSubImage3D(TType, mipLevel, 0, 0, 0, m_size[0], m_size[1], m_size[2], static_cast<GLenum>(format), static_cast<GLenum>(type), data);
				break;
			}
		}
		void update(SetDataFormat format, SetDataType type, const void* data, GLuint layer, GLuint mipLevel = 0)
		{
			bind();
			switch (TType)
			{
			case GL_TEXTURE_CUBE_MAP:
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, mipLevel, 0, 0, m_size[0], m_size[1], static_cast<GLenum>(format), static_cast<GLenum>(type), data);
				break;
			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_CUBE_MAP_ARRAY:
				glTexSubImage3D(TType, mipLevel, 0, 0, mipLevel, m_size[0], m_size[1], 1, static_cast<GLenum>(format), static_cast<GLenum>(type), data);
				break;
			}
		}

		void generateMipmaps()
		{
			bind();
			glGenerateMipmap(TType);
			// calculate number of mipmaps
			m_mipLevels = computeMaxMipMapLevels();
		}

		template<class T>
		void clear(const T& texel, SetDataFormat format, SetDataType type, GLuint level = 0)
		{
			glClearTexImage(m_id, level, static_cast<GLenum>(format), static_cast<GLenum>(type), &texel);
		}

		GLsizei width() const
		{
			return m_size[0];
		}

		template<bool TEnabled = TComponents >= 2>
		std::enable_if_t<TEnabled, GLsizei> height() const
		{
			return m_size[1];
		}

		template<bool TEnabled = TComponents >= 3>
		std::enable_if_t<TEnabled, GLsizei> depth() const
		{
			return m_size[2];
		}

		template<bool TEnabled = TComponents == 1>
		std::enable_if_t<TEnabled> resize(GLsizei width)
		{
			m_size[0] = width;
			allocateMemory();
		}

		template<bool TEnabled = TComponents == 2>
		std::enable_if_t<TEnabled> resize(GLsizei width, GLsizei height)
		{
			m_size[0] = width;
			m_size[1] = height;
			allocateMemory();
		}

		template<bool TEnabled = TComponents == 3>
		std::enable_if_t<TEnabled> resize(GLsizei width, GLsizei height, GLsizei depth)
		{
			m_size[0] = width;
			m_size[1] = height;
			m_size[2] = depth;
			allocateMemory();
		}

		InternalFormat format() const
		{
			return m_internalFormat;
		}

		GLuint getNumLevels() const
		{
			return m_mipLevels;
		}

		GLuint getId() const
		{
			return m_id;
		}

#ifdef GL_TEXTURE_BINDLESS
		void makeResident()
		{
			assert(m_lastHandle == 0);
			m_lastHandle = glGetTextureHandleARB(m_id);
			glMakeTextureHandleResidentARB(m_lastHandle);
		}
		void makeResident(const Sampler& sampler)
		{
			assert(m_lastHandle == 0);
			m_lastHandle = glGetTextureSamplerHandleARB(m_id, sampler.getId());
			glMakeTextureHandleResidentARB(m_lastHandle);
		}
		void makeNonResident()
		{
			assert(m_lastHandle != 0);
			glMakeTextureHandleNonResidentARB(m_lastHandle);
			m_lastHandle = 0;
		}
#endif
		GLsizei size() const
		{
			return std::accumulate(m_size.begin(), m_size.end(), GLsizei(1), [](GLsizei left, GLsizei right)
			{
				return left * right;
			});
		}
		template<class T>
		std::vector<T> getData(GLint mipLevel, SetDataFormat format, SetDataType type) const
		{
			glBindTexture(TType, m_id);
			std::vector<T> data;
			data.resize(size());
			glGetTexImage(TType, mipLevel, GLenum(format), GLenum(type), data.data());
			return data;
		}
	private:
		void bind() const
		{
			assert(m_id);
			glBindTexture(TType, m_id);
		}
		void allocateMemory()
		{
			bind();
			switch (TType)
			{
			case GL_TEXTURE_1D:
				glTexStorage1D(TType, m_mipLevels, static_cast<GLenum>(m_internalFormat.value), m_size[0]);
				break;
			case GL_TEXTURE_2D:
			case GL_TEXTURE_CUBE_MAP:
				glTexStorage2D(TType, m_mipLevels, static_cast<GLenum>(m_internalFormat.value), m_size[0], m_size[1]);
				break;
			case GL_TEXTURE_3D:
			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_CUBE_MAP_ARRAY:
				glTexStorage3D(TType, m_mipLevels, static_cast<GLenum>(m_internalFormat.value), m_size[0], m_size[1], m_size[2]);
				break;
			}
		}
		GLuint computeMaxMipMapLevels()
		{
			GLuint maxMip = 1;
			GLuint maxResolution = std::accumulate(m_size.begin(), m_size.end(), GLuint(0), [](GLuint a, GLuint b)
			{
				return std::max(a, b);
			});
			while ((maxResolution /= 2) > 0) ++maxMip;
			return maxMip;
		}
	private:
		unique<GLuint> m_id;
		unique<InternalFormat> m_internalFormat;
		std::array<unique<GLsizei>, TComponents> m_size;
		unique<GLuint, 1> m_mipLevels;

#ifdef GL_TEXTURE_BINDLESS
		unique<GLuint64> m_lastHandle;
#endif
	};

	using Texture1D = Texture<GL_TEXTURE_1D, 1>;
	
	using Texture2D = Texture<GL_TEXTURE_2D, 2>;

	using Texture3D = Texture<GL_TEXTURE_3D, 3>;

	using TextureCubeMap = Texture<GL_TEXTURE_CUBE_MAP, 2>;

	using Texture2DArray = Texture<GL_TEXTURE_2D_ARRAY, 3>;

	using TextureCubeMapArray = Texture<GL_TEXTURE_CUBE_MAP_ARRAY, 3>;
}
