#pragma once
#include "id.h"
#include "../opengl.h"
#include "format.h"
#include <array>

namespace gl
{
	template<GLenum TType, GLsizei TComponents>
	class Texture
	{
	public:
		Texture() = default;
		template<bool TEnabled = TComponents == 1>
		explicit Texture(InternalFormat internalFormat, std::enable_if_t<TEnabled, GLsizei> width)
			:
		Texture(internalFormat)
		{
			m_size[0] = width;
			allocateMemory();
		}
		template<bool TEnabled = TComponents == 2>
		explicit Texture(InternalFormat internalFormat, std::enable_if_t<TEnabled, GLsizei> width, GLsizei height)
			:
		Texture(internalFormat)
		{
			m_size[0] = width;
			m_size[1] = height;
			allocateMemory();
		}
		template<bool TEnabled = TComponents == 3>
		explicit Texture(InternalFormat internalFormat, std::enable_if_t<TEnabled, GLsizei> width, GLsizei height, GLsizei depth)
			:
		Texture(internalFormat)
		{
			m_size[0] = width;
			m_size[1] = height;
			m_size[2] = depth;
			allocateMemory();
		}

		explicit Texture(InternalFormat internalFormat)
			:
		m_internalFormat(internalFormat)
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

		void bindAsImage(GLuint slot, GLuint mipLevel, ImageAccess access) const
		{
			glBindImageTexture(slot, m_id, mipLevel, GL_TRUE, 0, static_cast<GLenum>(access), static_cast<GLenum>(m_internalFormat.value));
		}

		void bindAsImage(GLuint slot, GLuint mipLevel, ImageAccess access, GLint layer) const
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
	private:
		void bind() const
		{
			glBindTexture(TType, m_id);
		}
		void allocateMemory()
		{
			bind();
			switch (TType)
			{
			case GL_TEXTURE_1D:
				glTexStorage1D(TType, m_mipLevels, m_internalFormat, m_size[0]);
				break;
			case GL_TEXTURE_2D:
			case GL_TEXTURE_CUBE_MAP:
				glTexStorage2D(TType, m_mipLevels, m_internalFormat, m_size[0], m_size[1]);
				break;
			case GL_TEXTURE_3D:
			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_CUBE_MAP_ARRAY:
				glTexStorage3D(TType, m_mipLevels, m_internalFormat, m_size[0], m_size[1], m_size[2]);
				break;
			}
		}
	private:
		unique<GLuint> m_id;
		unique<InternalFormat> m_internalFormat;
		std::array<unique<GLsizei>, TComponents> m_size;
		unique<GLuint, 1> m_mipLevels;
	};

	using Texture1D = Texture<GL_TEXTURE_1D, 1>;
	
	using Texture2D = Texture<GL_TEXTURE_2D, 2>;

	using Texture3D = Texture<GL_TEXTURE_3D, 3>;

	using TextureCubeMap = Texture<GL_TEXTURE_CUBE_MAP, 2>;

	using Texture2DArray = Texture<GL_TEXTURE_2D_ARRAY, 3>;

	using TextureCubeMapArray = Texture<GL_TEXTURE_CUBE_MAP_ARRAY, 3>;
}
