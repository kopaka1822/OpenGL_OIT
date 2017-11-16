#pragma once
#include "IModel.h"
#include "IShader.h"
#include "ICamera.h"

class IRenderer
{
public:
	virtual ~IRenderer(){}
	virtual void render(const IModel* model, IShader* shader, const ICamera* camera) = 0;
};
