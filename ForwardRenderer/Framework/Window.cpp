#include "Window.h"
#include <glad/glad.h>
#include "DebugContext.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include "IKeyReceiver.h"
#include "IMouseReceiver.h"
#include "IWindowReceiver.h"
#include <cassert>
#include "../ScriptEngine/ScriptEngine.h"

static std::vector<IKeyReceiver*> s_keyReceiver;
static std::vector<IMouseReceiver*> s_mouseReceiver;
static std::vector<IWindowReceiver*> s_windowReceiver;
static int s_windowWidth = 0;
static int s_windowHeight = 0;
static Window* s_wnd = nullptr;

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

void Window::windowSizeFunc(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0)
		return;

	s_windowWidth = width;
	s_windowHeight = height;
	glViewport(0, 0, width, height);
	s_wnd->resetState();

	for (const auto& r : s_windowReceiver)
		r->onSizeChange(width, height);
}

Window::Window(size_t width, size_t height, const std::string& title)
{
	assert(!s_wnd);
	s_wnd = this;
	glfwSetErrorCallback(errorCallbackGLFW);
	if (!glfwInit()) throw std::runtime_error("Cannot initialize GLFW!\n");
	std::cerr << "INF: Creating window and context...\n";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DEPTH_BITS, 32);
	glfwWindowHint(GLFW_STENCIL_BITS, 8);
#ifndef _NO_GL_DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
	 
	m_handle = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
	s_windowWidth = int(width);
	s_windowHeight = int(height);
	if (!m_handle) throw std::runtime_error("Window creation failed!");

	glfwMakeContextCurrent(m_handle);

	glfwSetCursorPosCallback(m_handle, mouseFunc);
	glfwSetKeyCallback(m_handle, keyFunc);
	glfwSetMouseButtonCallback(m_handle, mouseButtonFunc);
	glfwSetScrollCallback(m_handle, mouseScrollFunc);
	glfwSetWindowSizeCallback(m_handle, windowSizeFunc);

	if (!gladLoadGL())
		throw std::exception("Cannot initialize Glad/load gl-function pointers!\n");
	std::cerr << "INF: Loaded GL-context is version " << GLVersion.major << '.' << GLVersion.minor << '\n';

#ifndef _NO_GL_DEBUG
	DebugContext::Init();
#endif

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	ScriptEngine::addProperty("windowSize", [this]()
	{
		int w, h;
		glfwGetWindowSize(m_handle, &w, &h);
		return std::to_string(w) + ", " + std::to_string(h);
	}, [this](const std::vector<Token>& args)
	{
		auto w = std::max(1, args.at(0).getInt());
		auto h = std::max(1, args.at(1).getInt());
		glfwSetWindowSize(m_handle, w, h);
	});
}

Window::~Window()
{
	if (m_handle)
		glfwDestroyWindow(m_handle);
	glfwTerminate();
	s_wnd = nullptr;
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
	resetState();
}

void Window::setTitle(const std::string& title)
{
	glfwSetWindowTitle(m_handle, title.c_str());
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
	const auto end = std::remove_if(s_mouseReceiver.begin(), s_mouseReceiver.end(), [recv](const IMouseReceiver* i)
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
	const auto end = std::remove_if(s_windowReceiver.begin(), s_windowReceiver.end(), [recv](const IWindowReceiver* i)
	{
		return (i == recv);
	});
	s_windowReceiver.erase(end, s_windowReceiver.end());
}

void Window::resetState() const
{
	glUseProgram(0);
}
