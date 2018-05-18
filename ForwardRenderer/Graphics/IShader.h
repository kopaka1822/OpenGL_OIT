#pragma once
#include <glm/matrix.hpp>
#include "../Framework/ParamSet.h"

class ICamera;

class IShader
{
public:
	virtual ~IShader(){}
	virtual void bind() const = 0;

	virtual void applyCamera(const ICamera& camera) {}
	virtual void setModelTransform(const glm::mat4& transform) {}

	virtual void setMaterial(const ParamSet& material){}
};
