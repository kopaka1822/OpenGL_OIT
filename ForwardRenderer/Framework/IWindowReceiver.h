#pragma once

// receives events about window properties
class IWindowReceiver
{
public:
	IWindowReceiver()
	{
		Window::registerWindowReceiver(this);
	}
	virtual ~IWindowReceiver()
	{
		Window::unregisterWindowReceiver(this);
	}
	virtual void onSizeChange(int width, int height) = 0;
};