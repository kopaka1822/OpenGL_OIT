#pragma once
#include <glad/glad.h>
#include <glm/detail/type_vec4.hpp>
#include <string>
#include <memory>
#include <vector>
#include <glad/glad.h>

class Texture2D
{
	// create texture with a single color
	Texture2D(const glm::vec4& color);
	// load texture from file
	Texture2D(const std::string& filename);

	/**
	 * \brief create texture and uploads data. see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
	 *  + sets member variables
	 * \param internalFormat number of color components
	 * \param width width in texel
	 * \param height height in texel
	 * \param format (for uncompressed) format of pixel data. e.g. GL_RGBA
	 * \param type (for uncompressed) texel type. e.g. GL_BYTE
	 * \param data image data
	 * \param compressedSize (for compressed) size of the compressed data. should be 0 if image is not compressed
	 */
	void loadTexture(GLenum internalFormat, GLsizei width, GLsizei height, GLenum format,
		GLenum type, const void* data, bool mipmaps, GLsizei compressedSize = 0);
public:
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	/**
	* \brief 
	* \param internalFormat
	* \param format format of pixel data. e.g. GL_RGBA
	* \param width width in texel
	* \param height height in texel
	* \param type texel type. e.g. GL_BYTE
	* \param data image data
	*/
	Texture2D(GLenum internalFormat, GLenum format, GLsizei width, GLsizei height, GLenum type, bool mipmaps, const void* data)
	{
		loadTexture(internalFormat, width, height, format, type, data, mipmaps);
	}

	void update(const void* data)
	{
		glBindTexture(GL_TEXTURE_2D, m_id);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
			m_width, m_height,
			m_format, m_type, data);
	}

	/**
	* \brief
	* \param index binding index
	* \param imageFormat image format like: GL_RG32F. see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBindImageTexture.xhtml
	*/
	void bindAsImage(GLuint index, GLenum imageFormat)
	{
		glBindImageTexture(index, m_id, 0, GL_FALSE, 0, GL_READ_WRITE, imageFormat);
	}

	void bind(GLuint index) const;
	bool isTransparent() const
	{
		return m_isTransparent;
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
		res.resize(m_width * m_height);
		glBindTexture(GL_TEXTURE_2D, m_id);
		glGetTexImage(GL_TEXTURE_2D, 0, m_format, m_type, res.data());
		return res;
	}

	GLuint getId() const
	{
		return m_id;
	}

	// load texture from file
	static std::shared_ptr<Texture2D> loadFromFile(const std::string& filename);
	// create texture with a single color
	static std::shared_ptr<Texture2D> loadConstant(const glm::vec4& color);

	// empties the cache. note: textures will only be deleted if no shared pointers use them
	static void clearCache();

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

	~Texture2D();
	
private:
	GLuint m_id = 0;
	bool m_isTransparent = false;
	GLsizei m_width = 0, m_height = 0;
	GLenum m_format = 0, m_type = 0;
};
