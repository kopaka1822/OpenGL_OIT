#include "Application.h"
#include "DebugContext.h"
#include "SimpleForwardRenderer.h"
#include "ObjModel.h"
#include "SimpleShader.h"
#include "ProjectionCamera.h"
#include "ITickReveicer.h"
#include <algorithm>
#include <chrono>

std::vector<ITickReceiver*> s_tickReceiver;

Application::Application()
	:
	m_window(800, 600, "ForwardRenderer")
{
	DebugContext context;

	// TODO set this via script
	m_renderer = std::make_unique<SimpleForwardRenderer>();
	//m_model = std::make_unique<ObjModel>("Scene/cornell_box.obj");
	//m_model = std::make_unique<ObjModel>("Scene/miguel/san-miguel-low-poly.obj");
	m_model = std::make_unique<ObjModel>("Scene/sponza/sponza.obj");
	m_shader = std::make_unique<SimpleShader>();
	m_camera = std::make_unique<ProjectionCamera>(40.0f, 1.0f);
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
