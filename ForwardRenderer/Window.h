#pragma once
#include <string>

class IKeyReceiver;
class IMouseReceiver;
class IWindowReceiver;

class Window
{
public:
	Window(size_t width, size_t height, const std::string& title);
	~Window();
	bool isOpen() const { return m_open; }
	void handleEvents();
	void swapBuffer() const;
	void setTitle(const std::string& title);

	static int getWidth();
	static int getHeight();

	static void registerKeyReceiver(IKeyReceiver* recv);
	static void unregisterKeyReceiver(IKeyReceiver* recv);
	static void registerMouseReceiver(IMouseReceiver* recv);
	static void unregisterMouseReceiver(IMouseReceiver* recv);
	static void registerWindowReceiver(IWindowReceiver* recv);
	static void unregisterWindowReceiver(IWindowReceiver* recv);
private:
	struct GLFWwindow* m_handle = nullptr;
	bool m_open = true;
};

