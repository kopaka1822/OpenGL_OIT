#include "Window.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Framework/IKeyReceiver.h"
#include "Framework/IMouseReceiver.h"
#include "Framework/IWindowReceiver.h"

static std::vector<IKeyReceiver*> s_keyReceiver;
static std::vector<IMouseReceiver*> s_mouseReceiver;
static std::vector<IWindowReceiver*> s_windowReceiver;
static int s_windowWidth = 0;
static int s_windowHeight = 0;

static void errorCallbackGLFW(int error, const char* description)
{
	std::cerr << "ERR: GLFW error, code " << error << " desc: \"" << description << "\"\n";
}

static void mouseFunc(GLFWwindow* window, double x, double y)
{
	static double oldX = 0.0, oldY = 0.0;

	for (const auto& r : s_mouseReceiver)
		r->onMouseMove(x, y, x - oldX, y - oldY);

	oldX = x, oldY = y;
}

static void keyFunc(GLFWwindow* window, int key, int scancode, int action, int modifier)
{
	if (action == GLFW_PRESS)
		for (const auto& r : s_keyReceiver)
			r->onKeyDown(key);
	else if(action == GLFW_RELEASE)
		for (const auto& r : s_keyReceiver)
			r->onKeyUp(key);
}

static void mouseButtonFunc(GLFWwindow* window, int button, int action, int modifier)
{
	if (action == GLFW_PRESS)
		for (const auto& r : s_mouseReceiver)
			r->onButtonDown(button);
	else if (action == GLFW_RELEASE)
		for (const auto& r : s_mouseReceiver)
			r->onButtonUp(button);
}

static void mouseScrollFunc(GLFWwindow* window, double x, double y)
{
	for (const auto& r : s_mouseReceiver)
		r->onScroll(x, y);
}

static void windowSizeFunc(GLFWwindow* window, int width, int height)
{
	s_windowWidth = width;
	s_windowHeight = height;
	glViewport(0, 0, width, height);

	for (const auto& r : s_windowReceiver)
		r->onSizeChange(width, height);
}

Window::Window(size_t width, size_t height, const std::string& title)
{
	glfwSetErrorCallback(errorCallbackGLFW);
	if (!glfwInit()) throw std::runtime_error("Cannot initialize GLFW!\n");
	std::cerr << "INF: Creating window and context...\n";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DEPTH_BITS, 32);
	glfwWindowHint(GLFW_STENCIL_BITS, 0);
	m_handle = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
	s_windowWidth = width;
	s_windowHeight = height;
	if (!m_handle) throw std::runtime_error("Window creation failed!");

	glfwMakeContextCurrent(m_handle);

	glfwSetCursorPosCallback(m_handle, mouseFunc);
	glfwSetKeyCallback(m_handle, keyFunc);
	glfwSetMouseButtonCallback(m_handle, mouseButtonFunc);
	glfwSetScrollCallback(m_handle, mouseScrollFunc);
	glfwSetWindowSizeCallback(m_handle, windowSizeFunc);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
}

Window::~Window()
{
	if (m_handle)
		glfwDestroyWindow(m_handle);
	glfwTerminate();
}

void Window::handleEvents()
{
	glfwPollEvents();
	m_open = !glfwWindowShouldClose(m_handle);
}

void Window::swapBuffer() const
{
	glFlush();
	glfwSwapBuffers(m_handle);
}

int Window::getWidth()
{
	return s_windowWidth;
}

int Window::getHeight()
{
	return s_windowHeight;
}

void Window::registerKeyReceiver(IKeyReceiver* recv)
{
	s_keyReceiver.push_back(recv);
}

void Window::unregisterKeyReceiver(IKeyReceiver* recv)
{
	auto end = std::remove_if(s_keyReceiver.begin(), s_keyReceiver.end(), [recv](const IKeyReceiver* i)
	{
		return (i == recv);
	});
	s_keyReceiver.erase(end, s_keyReceiver.end());
}

void Window::registerMouseReceiver(IMouseReceiver* recv)
{
	s_mouseReceiver.push_back(recv);
}

void Window::unregisterMouseReceiver(IMouseReceiver* recv)
{
	auto end = std::remove_if(s_mouseReceiver.begin(), s_mouseReceiver.end(), [recv](const IMouseReceiver* i)
	{
		return (i == recv);
	});
	s_mouseReceiver.erase(end, s_mouseReceiver.end());
}

void Window::registerWindowReceiver(IWindowReceiver* recv)
{
	s_windowReceiver.push_back(recv);
}

void Window::unregisterWindowReceiver(IWindowReceiver* recv)
{
	auto end = std::remove_if(s_windowReceiver.begin(), s_windowReceiver.end(), [recv](const IWindowReceiver* i)
	{
		return (i == recv);
	});
	s_windowReceiver.erase(end, s_windowReceiver.end());
}
