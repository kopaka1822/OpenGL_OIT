#include "CachedTexture2D.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../Dependencies/stb_image.h"
#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include <glad/glad.h>

std::unordered_map<glm::vec4, std::shared_ptr<CachedTexture2D>> s_cachedConstantTextures;
std::unordered_map<std::string, std::shared_ptr<CachedTexture2D>> s_cachedTextures;

CachedTexture2D::CachedTexture2D(const glm::vec4& color)
{
	loadTexture(GL_RGBA, 1, 1, GL_RGBA, GL_FLOAT, &color, true);
	if (color.a < 1.0f)
		m_isTransparent = true;
}

static GLenum getFormatFromComponents(int numComponents)
{
	switch(numComponents)
	{
	case 1: return GL_RED;
	case 2: return GL_RG;
	case 3: return GL_RGB;
	case 4: return GL_RGBA;
	}
	return GL_INVALID_ENUM;
}

CachedTexture2D::CachedTexture2D(const std::string& filename)
{
	int width = 0, height = 0, numComponents = 0;
	
	stbi_set_flip_vertically_on_load(true);
	auto data = stbi_load(filename.c_str(), &width, &height, &numComponents, 0);
	if (!data)
		throw std::runtime_error("cannot load texture " + filename);

	auto format = getFormatFromComponents(numComponents);
	loadTexture(format, width, height, format, GL_UNSIGNED_BYTE, data, true);

	// determine if transparent
	if(numComponents == 4)
	{
		auto bytes = reinterpret_cast<unsigned char*>(data);
		auto end = bytes + width * height * numComponents;
		while(bytes != end)
		{
			// alpha
			if(bytes[3] != 255)
			{
				m_isTransparent = true;
				break;
			}
			bytes += 4; // next pixel
		}
	}

	stbi_image_free(data);
}

void CachedTexture2D::loadTexture(GLenum internalFormat, GLsizei width, GLsizei height,
	GLenum format, GLenum type, const void* data, bool mipmaps, GLsizei compressedSize)
{
	m_width = width;
	m_height = height;
	m_format = format;
	m_type = type;

	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);

	if(compressedSize)
	{
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
			compressedSize, data);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
			format, type, data);
	}

	if(mipmaps)
		glGenerateMipmap(GL_TEXTURE_2D);

	if (mipmaps)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void CachedTexture2D::bind(GLuint index) const
{
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(GL_TEXTURE_2D, m_id);
}

std::shared_ptr<CachedTexture2D> CachedTexture2D::loadFromFile(const std::string& filename)
{
	// cached?
	auto it = s_cachedTextures.find(filename);
	if (it != s_cachedTextures.end())
		return it->second;

	// create new one
	std::shared_ptr<CachedTexture2D> tex;
	tex.reset(new CachedTexture2D(filename));
	s_cachedTextures[filename] = tex;
	return tex;
}

std::shared_ptr<CachedTexture2D> CachedTexture2D::loadConstant(const glm::vec4& color)
{
	// cached?
	auto it = s_cachedConstantTextures.find(color);
	if (it != s_cachedConstantTextures.end())
		return it->second;

	// create new one
	std::shared_ptr<CachedTexture2D> tex;
	tex.reset(new CachedTexture2D(color));
	s_cachedConstantTextures[color] = tex;
	return tex;
}

void CachedTexture2D::clearCache()
{
	s_cachedConstantTextures.clear();
	s_cachedTextures.clear();
}

CachedTexture2D::~CachedTexture2D()
{
	glDeleteTextures(1, &m_id);
}
