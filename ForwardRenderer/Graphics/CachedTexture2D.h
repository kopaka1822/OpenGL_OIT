#pragma once
#include <glad/glad.h>
#include <glm/detail/type_vec4.hpp>
#include <string>
#include <memory>
#include <vector>
#include "../Dependencies/gl/texture.hpp"

class CachedTexture2D : public gl::Texture2D
{
	// create texture with a single color
	CachedTexture2D(const glm::vec4& color);
	// load texture from file
	CachedTexture2D(const std::string& filename);
public:
	CachedTexture2D(const CachedTexture2D&) = delete;
	CachedTexture2D& operator=(const CachedTexture2D&) = delete;
	~CachedTexture2D() = default;

	bool isTransparent() const
	{
		return m_isTransparent;
	}

	// load texture from file
	static std::shared_ptr<CachedTexture2D> loadFromFile(const std::string& filename);
	// create texture with a single color
	static std::shared_ptr<CachedTexture2D> loadConstant(const glm::vec4& color);

	// empties the cache. note: textures will only be deleted if no shared pointers use them
	static void clearCache();

	
	
private:
	bool m_isTransparent = false;
};
