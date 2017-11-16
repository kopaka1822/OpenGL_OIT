#pragma once
#include <string>
class IKeyReceiver;
class IMouseReceiver;

class Window
{
public:
	Window(size_t width, size_t height, const std::string& title);
	~Window();
	bool isOpen() const { return m_open; }
	void handleEvents();
	void swapBuffer() const;

	static void registerKeyReceiver(IKeyReceiver* recv);
	static void unregisterKeyReceiver(IKeyReceiver* recv);
	static void registerMouseReceiver(IMouseReceiver* recv);
	static void unregisterMouseReceiver(IMouseReceiver* recv);
private:
	struct GLFWwindow* m_handle = nullptr;
	bool m_open = true;
};

