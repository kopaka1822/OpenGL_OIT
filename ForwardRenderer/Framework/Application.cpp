#include "Application.h"
#include "DebugContext.h"
#include "../SimpleForwardRenderer.h"
#include "../ObjModel.h"
#include "../SimpleShader.h"
#include "../ProjectionCamera.h"
#include "ITickReveicer.h"
#include <algorithm>
#include <chrono>
#include "../ScriptEngine/Token.h"
#include "../ScriptEngine/ScriptEngine.h"
#include "../AdaptiveTransparencyRenderer.h"
#include "../WeightedTransparency.h"
#include "../LinkedVisibility.h"
#include "Profiler.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../Dependencies/stb_image_write.h"

#include "../Dependencies/stb_image.h"
#include "../Dependencies/stbi_helper.h"

std::vector<ITickReceiver*> s_tickReceiver;

static std::string s_rendererName;
static std::string s_cameraName;

static std::unique_ptr<IRenderer> makeRenderer(const std::vector<Token>& args)
{
	if (args.empty())
		throw std::runtime_error("renderer name missing");

	const auto name = args[0].getString();

	if(name == "forward")
		return std::make_unique<SimpleForwardRenderer>();
	if (name == "adaptive")
		return std::make_unique<AdaptiveTransparencyRenderer>();
	if (name == "weighted_oit")
		return std::make_unique<WeightedTransparency>();
	if (name == "linked")
		return std::make_unique<LinkedVisibility>();

	throw std::runtime_error("renderer not found");
}

static std::unique_ptr<ICamera> makeCamera(const std::vector<Token>& args)
{
	if (args.empty())
		throw std::runtime_error("camera name missing");

	const auto name = args[0].getString();

	if (name == "projection")
		return std::make_unique<ProjectionCamera>();

	throw std::runtime_error("camera not found");
}

Application::Application()
	:
	m_window(800, 600, "ForwardRenderer")
{

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

	ScriptEngine::addFunction("loadObj", [this](const std::vector<Token>& args)
	{
		if (args.empty())
			throw std::runtime_error("filename missing");

		m_model = std::make_unique<ObjModel>(args[0].getString());
	});

	ScriptEngine::addFunction("makeScreenshot",[this](const std::vector<Token>& args)
	{
		if (args.empty())
			throw std::runtime_error("filename missing");

		m_screenshotDestination = args.at(0).getString();
	});

	ScriptEngine::addFunction("makeDiff", [this](const std::vector<Token>& args)
	{
		if (args.size() < 3)
			throw std::runtime_error("expected at least three arguments. source1, source2, destination [, factor]");
		float factor = 1.0f;
		if (args.size() >= 4)
			factor = args.at(3).getFloat();

		makeDiff(args.at(0).getString(), args.at(1).getString(), args.at(2).getString(), factor);
	});

	ICamera::initScripts();

	m_shader = std::make_unique<SimpleShader>(SimpleShader::getLinkedDefaultProgram());
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
	
	if(!m_screenshotDestination.empty())
	{
		makeScreenshot(m_screenshotDestination);
		m_screenshotDestination.clear();
	}

	m_window.swapBuffer();

	// adjust window title
	auto profile = Profiler::getActive();
	m_window.setTitle(s_rendererName + " " + std::get<0>(profile) + ": " + std::to_string(std::get<1>(profile))
		+ " iterations: " + std::to_string(ScriptEngine::getIteration())
	);
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

void Application::makeScreenshot(const std::string& filename)
{
	const auto width = Window::getWidth();
	const auto height = Window::getHeight();
	std::vector<uint8_t> data;
	data.resize(width * height * 3);
	
	// obtain data from backbuffer
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.data());

	stbi_flip_vertically_on_write(1);
	const auto res = stbi_write_png(filename.c_str(), width, height, 3, data.data(), 0);
	if(!res)
		std::cerr << "could not save screenshot\n";
	else
		std::cout << "saved " << filename << '\n';
}

void Application::makeDiff(const std::string& src1, const std::string& src2, const std::string& dst, float factor)
{
	int width1 = 0, width2 = 0, height1 = 0, height2 = 0, channels1 = 0, channels2 = 0;
	
	stbi_ptr pic1(stbi_load(src1.c_str(), &width1, &height1, &channels1, 3));
	if (!pic1)
		throw std::runtime_error("could not open " + src1);

	stbi_ptr pic2(stbi_load(src2.c_str(), &width2, &height2, &channels2, 3));
	if (!pic2)
		throw std::runtime_error("could not open " + src2);

	if (width1 != width2 || height1 != height2)
		throw std::runtime_error(src1 + " and " + src2 + " have not the same dimensions");

	// save transformed image into pic1
	const size_t size = width1 * height1 * 3;
	const auto it1 = stdext::make_checked_array_iterator(pic1.get(), size);
	const auto it2 = stdext::make_checked_array_iterator(pic2.get(), size);
	std::transform(it1, it1 + size, it2, it1, [factor](unsigned char b1, unsigned char b2)
	{
		return static_cast<unsigned char>(std::min(255.0f, std::abs(b1 - b2) * factor));
	});

	// save image
	stbi_flip_vertically_on_write(1);
	if (!stbi_write_png(dst.c_str(), width1, height1, 3, pic1.get(), 0))
		std::cerr << "could not save diff\n";
	else
		std::cout << "saved diff\n";
}
