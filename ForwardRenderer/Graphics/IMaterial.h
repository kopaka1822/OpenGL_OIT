#pragma once
#include <glm/detail/type_vec4.hpp>
#include <string>
#include "CachedTexture2D.h"

class IMaterial
{
public:
	virtual ~IMaterial() {}
	virtual const glm::vec4* getAttribute(const std::string& name) const = 0;
	virtual const CachedTexture2D* getTexture(const std::string& name) const = 0;
};
