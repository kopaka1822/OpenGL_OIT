#pragma once
#include "Window.h"

class IKeyReceiver
{
public:
	IKeyReceiver()
	{
		Window::registerKeyReceiver(this);
	}
	virtual ~IKeyReceiver()
	{
		Window::unregisterKeyReceiver(this);
	}

	virtual void onKeyDown(int key) = 0;
	virtual void onKeyUp(int key) = 0;
};