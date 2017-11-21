#pragma once
#include <glad/glad.h>

class Texture3D
{
public:
	/**
	 * \brief 
	 * \param width width in texel
	 * \param height height in texel
	 * \param depth depth in texel
	 * \param type data type GL_FLOAT, GL_BYTE
	 * \param linearInterpolation linear interpolation when reading data or nearest neighbor
	 * \param data texel
	 */
	Texture3D(GLenum internalFormat, GLenum format, size_t width, size_t height, size_t depth, 
		GLenum type, bool linearInterpolation, const void* data = nullptr)
		:
	m_width(width), m_height(height), m_depth(depth),
	m_format(format),
	m_type(type)
	{
		glGenTextures(1, &m_id);
		glBindTexture(GL_TEXTURE_3D, m_id);
		glTexImage3D(GL_TEXTURE_3D, 0, internalFormat,
			static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth)
			, 0, format, type, data);

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, linearInterpolation?GL_LINEAR:GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, linearInterpolation?GL_LINEAR:GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	}
	~Texture3D()
	{
		glDeleteTextures(1, &m_id);
	}
	void update(const void* data)
	{
		glBindTexture(GL_TEXTURE_3D, m_id);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
			static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height), static_cast<GLsizei>(m_depth)
			, m_format, m_type, data);
	}
	void bind(GLuint index) const
	{
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(GL_TEXTURE_3D, m_id);
	}

	/**
	 * \brief 
	 * \param index binding index
	 * \param imageFormat image format like: GL_RG32F. see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindImageTexture.xhtml
	 */
	void bindAsImage(GLuint index, GLenum imageFormat) const
	{
		glBindImageTexture(index, m_id, 0, GL_TRUE, 0, GL_READ_WRITE, imageFormat);
	}

	/**
	* \brief gets the image data from the cpu
	* \tparam T type of the image. IMPORTANT must be the size of one texel
	* \return image data
	*/
	template<class T>
	std::vector<T> getImageData() const
	{
		std::vector<T> res;
		res.resize(m_width * m_height * m_depth);
		glBindTexture(GL_TEXTURE_3D, m_id);
		glGetTexImage(GL_TEXTURE_3D, 0, m_format, m_type, res.data());
		return res;
	}

	size_t width() const
	{
		return m_width;
	}
	size_t height() const
	{
		return m_height;
	}
	size_t depth() const
	{
		return m_depth;
	}

	/**
	 * \brief clears the texture with a constant color
	 * \tparam T must be the size of one texel
	 * \param texel clear color
	 */
	template<class T>
	void clear(const T& texel)
	{
		glClearTexImage(m_id, 0, m_format, m_type, &texel);
	}
private:
	static GLenum getFormatFromComponents(int numComponents)
	{
		switch (numComponents)
		{
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		}
		return GL_INVALID_ENUM;
	}
private:
	GLuint m_id;
	size_t m_width, m_height, m_depth;
	GLenum m_format, m_type;
};
