#pragma once
#include "../Framework/ParamSet.h"

class ILights
{
public:
	virtual ~ILights() = default;
	virtual void addLight(ParamSet light) = 0;
	virtual void removeLight(int index) = 0;
	virtual void upload() = 0;
	virtual void bind() const = 0;
	virtual std::string displayLights() = 0;

	virtual size_t numLights() const = 0;
	virtual const ParamSet& getLight(size_t index) const = 0;
};