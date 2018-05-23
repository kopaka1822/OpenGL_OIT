#pragma once
#include "IModel.h"
#include "ICamera.h"
#include "ITransforms.h"

class IEnvironmentMap
{
public:
	virtual ~IEnvironmentMap() = default;
	virtual void render(const IModel& model, IShader& shader, const ICamera& cam, ITransforms& transforms) = 0;
	virtual void bind() const = 0;
};