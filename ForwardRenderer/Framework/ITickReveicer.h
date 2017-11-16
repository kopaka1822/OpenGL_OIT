#pragma once
#include "Application.h"

class ITickReceiver
{
public:
	ITickReceiver()
	{
		Application::registerTickReceiver(this);
	}
	virtual ~ITickReceiver()
	{
		Application::unregisterTickReceiver(this);
	}
	virtual void tick(float dt) = 0;
};