#pragma once
#include "../Window.h"
#include <memory>
#include "../Graphics/IRenderer.h"
#include "../Graphics/IModel.h"
#include "../Graphics/IShader.h"
#include "../Graphics/ICamera.h"
#include "../Graphics/ILights.h"

class ITickReceiver;

class Application
{
public:
	Application();
	void tick();

	bool isRunning() const;

	static void registerTickReceiver(ITickReceiver* recv);
	static void unregisterTickReceiver(ITickReceiver* recv);

private:
	static void makeScreenshot(const std::string& filename);
	static void makeDiff(const std::string& src1, const std::string& src2, const std::string& dst, float factor);

private:
	Window m_window;
	std::unique_ptr<IRenderer> m_renderer;
	std::unique_ptr<IModel> m_model;
	std::unique_ptr<ICamera> m_camera;
	std::unique_ptr<ILights> m_lights;
	std::string m_screenshotDestination;
};
