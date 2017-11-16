#pragma once
#include "Window.h"

class IMouseReceiver
{
public:
	IMouseReceiver()
	{
		Window::registerMouseReceiver(this);
	}
	virtual ~IMouseReceiver()
	{
		Window::unregisterMouseReceiver(this);
	}

	virtual void onMouseMove(double x, double y, double dx, double dy) {}
	virtual void onButtonDown(int button) {}
	virtual void onButtonUp(int button) {}
	virtual void onScroll(double x, double y){}
};