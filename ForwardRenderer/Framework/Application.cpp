#include "Application.h"
#include <regex>
#include "ITickReveicer.h"
#include <algorithm>
#include <chrono>
#include "../ScriptEngine/Token.h"
#include "../ScriptEngine/ScriptEngine.h"
#include "Profiler.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../Dependencies/stb_image_write.h"

#include "../Dependencies/stb_image.h"
#include "../Dependencies/stbi_helper.h"
#include "../Renderer/AdaptiveTransparencyRenderer.h"
#include "../Renderer/MultiLayerAlphaRenderer.h"
#include "../Renderer/DynamicFragmentBuffer.h"
#include "../Renderer/LinkedVisibility.h"
#include "../Renderer/WeightedTransparency.h"
#include "../Renderer/SimpleForwardRenderer.h"
#include <iostream>
#include "../Implementations/ObjModel.h"
#include "../Implementations/ProjectionCamera.h"
#include "../Implementations/SimpleLights.h"
#include "../Implementations/SimpleTransforms.h"
#include "../Implementations/SimpleShader.h"
#include "../Renderer/EnvironmentRenderer.h"
#include "../Implementations/ShadowMaps.h"
#include "../Renderer/ShadowDebugRenderer.h"
#include <sstream>
#include "../Renderer/DebugRenderer.h"

std::vector<ITickReceiver*> s_tickReceiver;

static std::string s_rendererName;
static std::string s_cameraName;
static std::string s_lightsName;

static std::unique_ptr<IRenderer> makeRenderer(const std::vector<Token>& args)
{
	if (args.empty())
		throw std::runtime_error("renderer name missing");

	const auto name = args[0].getString();

	if(name == "forward")
		return std::make_unique<SimpleForwardRenderer>();
	if (name == "weighted_oit")
		return std::make_unique<WeightedTransparency>();
	if (name == "linked")
		return std::make_unique<LinkedVisibility>();
	if (name == "environment")
		return std::make_unique<EnvironmentRenderer>();
	if (name == "shadow_map")
		return std::make_unique<ShadowDebugRenderer>();
	if (name == "dynamic_fragment")
		return std::make_unique<DynamicFragmentBufferRenderer>();
	if (name == "debug_renderer")
		return std::make_unique<DebugRenderer>();
	{
		const std::regex rgx("adaptive[1-9][0-9]*");
		if (std::regex_match(name, rgx))
		{
			size_t num = std::stoi(name.substr(8));
			return std::make_unique<AdaptiveTransparencyRenderer>(num);
		}
	}
	{
		const std::regex rgx("multilayer_alpha[1-9][0-9]*");
		if(std::regex_match(name, rgx))
		{
			size_t num = std::stoi(name.substr(16));
			return std::make_unique<MultiLayerAlphaRenderer>(num);
		}
	}

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

static std::unique_ptr<ILights> makeLights(const std::vector<Token>& args)
{
	if (args.empty())
		throw std::runtime_error("lights name missing");

	if (args.at(0).getString() == "default")
		return std::make_unique<SimpleLights>();

	throw std::runtime_error("lights not found");
}

Application::Application()
	:
	m_window(800, 800, "ForwardRenderer")
{
	initScripts();

	// load default shader
	auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
	auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
	auto fragment = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/DefaultShader.fs", 430, "#define DISABLE_ENVIRONMENT");
	m_envmapShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({ vertex, geometry, fragment }));

	m_envmap = std::make_unique<EnvironmentMap>(512);
	m_lights = std::make_unique<SimpleLights>();
	m_transforms = std::make_unique<SimpleTransforms>();
	m_shadows = std::make_unique<ShadowMaps>(1024 * 8, 1024);
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

	if (m_lights && m_shadows && m_model && m_transforms)
	{
		m_lights->upload(*m_shadows, *m_model, *m_transforms);
	}

	if (m_transforms && m_camera)
		m_transforms->update(*m_camera);

	if (m_transforms)
		m_transforms->upload();

	RenderArgs args;
	args.model = m_model.get();
	args.camera = m_camera.get();
	args.lights = m_lights.get();
	args.transforms = m_transforms.get();
	args.environment = m_envmap.get();
	args.shadows = m_shadows.get();

	glViewport(0, 0, Window::getWidth(), Window::getHeight());
	if (m_renderer)
		m_renderer->render(args);
	
	if(!m_screenshotDestination.empty())
	{
		makeScreenshot(m_screenshotDestination);
		m_screenshotDestination.clear();
	}

	m_window.swapBuffer();

	// adjust window title
	auto profile = Profiler::getActive();
	std::ostringstream ss;
	ss << s_rendererName << " " << std::get<0>(profile) << ": " << std::to_string(std::get<1>(profile)) <<
		" iterations: " << ScriptEngine::getIteration();
	if (ScriptEngine::getWaitIteration())
		ss << " wait: " << ScriptEngine::getWaitIteration();
	m_window.setTitle(ss.str());
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

void Application::initScripts()
{
	ScriptEngine::addProperty("renderer", []()
	{
		return s_rendererName;
	}, [this](std::vector<Token>& args)
	{
		this->m_renderer = makeRenderer(args);

		if (this->m_renderer)
			this->m_renderer->init();

		s_rendererName = args[0].getString();
		// reset profiler times
		Profiler::reset();
	});

	ScriptEngine::addProperty("camera", []()
	{
		return s_cameraName;
	}, [this](std::vector<Token>& args)
	{
		this->m_camera = makeCamera(args);
		s_cameraName = args[0].getString();
	});

	ScriptEngine::addProperty("lights", []()
	{
		return s_lightsName;
	}, [this](const std::vector<Token>& args)
	{
		this->m_lights = makeLights(args);
		s_lightsName = args.at(0).getString();
	});

	ScriptEngine::addFunction("loadObj", [this](const std::vector<Token>& args)
	{
		if (args.empty())
			throw std::runtime_error("filename missing");

		m_model = std::make_unique<ObjModel>(args[0].getString());
		return "";
	});

	ScriptEngine::addFunction("makeScreenshot", [this](const std::vector<Token>& args)
	{
		if (args.empty())
			throw std::runtime_error("filename missing");

		m_screenshotDestination = args.at(0).getString();
		return "";
	});

	ScriptEngine::addFunction("makeDiff", [this](const std::vector<Token>& args)
	{
		if (args.size() < 3)
			throw std::runtime_error("expected at least three arguments. source1, source2, destination [, factor]");
		float factor = 1.0f;
		if (args.size() >= 4)
			factor = args.at(3).getFloat();

		makeDiff(args.at(0).getString(), args.at(1).getString(), args.at(2).getString(), factor);

		return "";
	});

	ScriptEngine::addFunction("addPointLight", [this](const std::vector<Token>& args)
	{
		if (args.size() < 3)
			throw std::runtime_error("expected posX, posY, posZ [, colorR, colorG, colorB [, quadraticAttenuation [, linearAttenuation]]]");
		if (!m_lights)
			throw std::runtime_error("no light model active");

		ParamSet params;
		params.add("position", glm::vec3(args.at(0).getFloat(), args.at(1).getFloat(), args.at(2).getFloat()));

		if (args.size() >= 6)
			params.add("color", glm::vec3(args.at(3).getFloat(), args.at(4).getFloat(), args.at(5).getFloat()));

		if (args.size() >= 7)
			params.add("linearAttenuation", args.at(6).getFloat());

		if (args.size() >= 8)
			params.add("quadraticAttenuation", args.at(7).getFloat());

		m_lights->addLight(std::move(params));

		return "";
	});

	ScriptEngine::addFunction("addDirectionalLight", [this](const std::vector<Token>& args)
	{
		if (args.size() < 3)
			throw std::runtime_error("expected dirX, dirY, dirZ [, colorR, colorG, colorB]");
		if (!m_lights)
			throw std::runtime_error("no light model active");

		ParamSet params;
		params.add("direction", glm::vec3(args.at(0).getFloat(), args.at(1).getFloat(), args.at(2).getFloat()));

		if (args.size() >= 6)
			params.add("color", glm::vec3(args.at(3).getFloat(), args.at(4).getFloat(), args.at(5).getFloat()));

		m_lights->addLight(std::move(params));

		return "";
	});

	ScriptEngine::addFunction("showLights", [this](const std::vector<Token>& args)
	{
		if (!m_lights)
			throw std::runtime_error("no light model active");

		return m_lights->displayLights();
	});

	ScriptEngine::addFunction("removeLight", [this](const std::vector<Token>& args)
	{
		if (!m_lights)
			throw std::runtime_error("no light model active");

		m_lights->removeLight(args.at(0).getInt());
		return "";
	});

	ScriptEngine::addFunction("refreshEnvironment", [this](const std::vector<Token>& args)
	{
		if (!(m_model && m_envmapShader && m_camera && m_transforms && m_lights && m_shadows))
			throw std::runtime_error("model, camera, lights, shadows and transforms must be loaded for envmaps");

		glm::vec3 pos;
		if(args.size() == 0)
		{
			pos = m_camera->getPosition();
		}
		else if(args.size() == 3)
		{
			pos.x = args.at(0).getFloat();
			pos.y = args.at(1).getFloat();
			pos.z = args.at(2).getFloat();
		}
		else throw std::runtime_error("either 0 or 3 arguments expected for environment map");
		
		m_lights->upload(*m_shadows, *m_model, *m_transforms);
		m_transforms->update(*m_camera);
		m_transforms->upload();

		m_lights->bind();
		m_shadows->bind();

		std::cerr << "rendering environment map\n";
		m_envmap->render(*m_model, *m_envmapShader, *m_camera, *m_transforms, pos);

		return "";
	});

	ScriptEngine::addFunction("getMaterialName", [this](const std::vector<Token>& args)
	{
		if (!m_model) throw std::runtime_error("no model active");
		return m_model->getMaterial().getMaterial(args.at(0).getInt()).get("name", std::string("<no name>"));
	});

	ScriptEngine::addKeyword("forward");
	ScriptEngine::addKeyword("weighted_oit");
	ScriptEngine::addKeyword("linked");
	ScriptEngine::addKeyword("dynamic_fragment");
	ScriptEngine::addKeyword("adaptive");
	ScriptEngine::addKeyword("multilayer_alpha");
	ScriptEngine::addKeyword("projection");
	ScriptEngine::addKeyword("environment");
	ScriptEngine::addKeyword("shadow_map");
	ScriptEngine::addKeyword("debug_renderer");

	ICamera::initScripts();
	IRenderer::initScripts();
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
	
	stbi_set_flip_vertically_on_load(0);
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
	stbi_flip_vertically_on_write(0);
	if (!stbi_write_png(dst.c_str(), width1, height1, 3, pic1.get(), 0))
		std::cerr << "could not save diff\n";
	else
		std::cout << "saved diff\n";
}
