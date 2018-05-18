#pragma once
#include "../Framework/ParamSet.h"

class ILights
{
public:
	virtual ~ILights() = default;
	virtual void addLight(ParamSet light) = 0;
	virtual void upload() = 0;
	virtual void bind() const = 0;
};