#pragma once
#include "RenderArgs.h"

class IRenderer
{
public:
	virtual ~IRenderer(){}
	// this method will be called after the last renderer was destructed (should be used for script engine registration)
	virtual void init() {}
	virtual void render(const RenderArgs& args) = 0;
};
