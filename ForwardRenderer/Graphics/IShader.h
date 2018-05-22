#pragma once
#include <glm/matrix.hpp>
#include "../Framework/ParamSet.h"

class ICamera;

class IShader
{
public:
	virtual ~IShader(){}
	virtual void bind() const = 0;
};
