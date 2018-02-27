#pragma once
#include "stb_image.h"
#include <memory>

struct stbi_deleter
{
	void operator()(unsigned char* data) const {
		stbi_image_free(data);
	}
};

using stbi_ptr = std::unique_ptr<unsigned char, stbi_deleter>;