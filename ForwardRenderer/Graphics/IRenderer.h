#pragma once
#include "IModel.h"
#include "IShader.h"
#include "ICamera.h"

class IRenderer
{
public:
	virtual ~IRenderer(){}
	// this method will be called after the last renderer was destructed (should be used for script engine registration)
	virtual void init() {}
	virtual void render(const IModel* model, const ICamera* camera) = 0;
};
