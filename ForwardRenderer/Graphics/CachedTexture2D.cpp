#include "CachedTexture2D.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../Dependencies/stb_image.h"
#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include <glad/glad.h>

std::unordered_map<glm::vec4, std::shared_ptr<CachedTexture2D>> s_cachedConstantTextures;
std::unordered_map<std::string, std::shared_ptr<CachedTexture2D>> s_cachedTextures;

CachedTexture2D::CachedTexture2D(const glm::vec4& color)
	:
Texture(gl::InternalFormat::RGBA8, 1, 1)
{
	update(gl::SetDataFormat::RGBA, gl::SetDataType::FLOAT, &color);
	
	if (color.a < 1.0f)
		m_isTransparent = true;
}

static gl::InternalFormat getSizedFormatFromComponents(int numComponents)
{
	switch(numComponents)
	{
	case 1: return gl::InternalFormat::R8;
	case 2: return gl::InternalFormat::RG8;
	case 3: return gl::InternalFormat::RGB8;
	case 4: return gl::InternalFormat::RGBA8;
	}
	return gl::InternalFormat(-1);
}

static gl::SetDataFormat getFormatFromComponents(int numComponents)
{
	switch (numComponents)
	{
	case 1: return gl::SetDataFormat::R;
	case 2: return gl::SetDataFormat::RG;
	case 3: return gl::SetDataFormat::RGB;
	case 4: return gl::SetDataFormat::RGBA;
	}
	return gl::SetDataFormat(-1);
}

CachedTexture2D::CachedTexture2D(const std::string& filename)
{
	int width = 0, height = 0, numComponents = 0;
	
	stbi_set_flip_vertically_on_load(true);
	auto data = stbi_load(filename.c_str(), &width, &height, &numComponents, 0);
	if (!data)
		throw std::runtime_error("cannot load texture " + filename);

	auto format = getSizedFormatFromComponents(numComponents);

	// reinitialize texture
	reinterpret_cast<gl::Texture2D&>(*this) = gl::Texture2D(format, width, height);
	update(getFormatFromComponents(numComponents), gl::SetDataType::UINT8, data);
	generateMipmaps();

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
