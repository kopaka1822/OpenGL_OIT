#include "Application.h"
#include "DebugContext.h"
#include "SimpleForwardRenderer.h"
#include "ObjModel.h"
#include "SimpleShader.h"
#include "ProjectionCamera.h"
#include "ITickReveicer.h"
#include <algorithm>
#include <chrono>
#include "ScriptEngine/Token.h"
#include "ScriptEngine/ScriptEngine.h"

std::vector<ITickReceiver*> s_tickReceiver;

static std::string s_rendererName;
static std::string s_cameraName;

static std::unique_ptr<IRenderer> makeRenderer(const std::vector<Token>& args)
{
	if (args.size() == 0)
		throw std::runtime_error("renderer name missing");

	auto name = args[0].getString();

	if(name == "forward")
		return std::make_unique<SimpleForwardRenderer>();

	throw std::runtime_error("renderer not found");
}

static std::unique_ptr<ICamera> makeCamera(const std::vector<Token>& args)
{
	if (args.size() == 0)
		throw std::runtime_error("camera name missing");

	auto name = args[0].getString();

	if (name == "projection")
		return std::make_unique<ProjectionCamera>(40.0f);

	throw std::runtime_error("camera not found");
}

Application::Application()
	:
	m_window(800, 600, "ForwardRenderer")
{
	DebugContext context;

	ScriptEngine::addProperty("renderer", []()
	{
		std::cout << "renderer: " << s_rendererName << std::endl;
	}, [this](std::vector<Token>& args)
	{
		this->m_renderer = makeRenderer(args);
		s_rendererName = args[0].getString();
	});

	ScriptEngine::addProperty("camera", []()
	{
		std::cout << "camera: " << s_cameraName << std::endl;
	}, [this](std::vector<Token>& args)
	{
		this->m_camera = makeCamera(args);
		s_cameraName = args[0].getString();
	});

	ScriptEngine::addFunction("loadObj", [this](std::vector<Token> args)
	{
		if (args.size() == 0)
			throw std::runtime_error("filename missing");

		m_model = std::make_unique<ObjModel>(args[0].getString());
	});

	m_shader = std::make_unique<SimpleShader>();
}

void Application::tick()
{
	static auto time_start = std::chrono::high_resolution_clock::now();

	m_window.handleEvents();
	
	auto time_end = std::chrono::high_resolution_clock::now();
	float dt = float(std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count()) / 1000.0f;
	time_start = time_end;

	for (const auto& r : s_tickReceiver)
		r->tick(dt);

	if (m_renderer)
		m_renderer->render(m_model.get(), m_shader.get(), m_camera.get());
	
	m_window.swapBuffer();
}

bool Application::isRunning() const
{
	return m_window.isOpen();
}

void Application::registerTickReceiver(ITickReceiver* recv)
{
	s_tickReceiver.push_back(recv);
}

void Application::unregisterTickReceiver(ITickReceiver* recv)
{
	auto end = std::remove_if(s_tickReceiver.begin(), s_tickReceiver.end(), [recv](const ITickReceiver* i)
	{
		return (i == recv);
	});
	s_tickReceiver.erase(end, s_tickReceiver.end());
}
