#pragma once
#include "ICamera.h"

class ITransforms
{
public:
	virtual ~ITransforms() = default;
	virtual void upload() = 0;
	virtual void bind() const = 0;

	virtual void update(const ICamera& camera) = 0;
	virtual void setModelTransform(const glm::mat4& model) = 0;
	virtual const glm::mat4& getModelTransform() = 0;
};
