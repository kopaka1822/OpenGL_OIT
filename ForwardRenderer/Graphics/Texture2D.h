#pragma once
#include <glad/glad.h>
#include <glm/detail/type_vec4.hpp>
#include <string>
#include <memory>

class Texture2D
{
	// create texture with a single color
	Texture2D(const glm::vec4& color);
	// load texture from file
	Texture2D(const std::string& filename);

	/**
	 * \brief create texture and uploads data. see: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
	 * \param internalFormat number of color components
	 * \param width width in texel
	 * \param height height in texel
	 * \param format (for uncompressed) format of pixel data. e.g. GL_RGBA
	 * \param type (for uncompressed) texel type. e.g. GL_BYTE
	 * \param data image data
	 * \param compressedSize (for compressed) size of the compressed data. should be 0 if image is not compressed
	 */
	void loadTexture(GLenum internalFormat, GLsizei width, GLsizei height, GLenum format,
		GLenum type, const void* data, GLsizei compressedSize = 0);
public:
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	void bind(GLuint index) const;

	// load texture from file
	static std::shared_ptr<Texture2D> loadFromFile(const std::string& filename);
	// create texture with a single color
	static std::shared_ptr<Texture2D> loadConstant(const glm::vec4& color);

	// empties the cache. note: textures will only be deleted if no shared pointers use them
	static void clearCache();

	~Texture2D();
	
private:
	GLuint m_id;
};
