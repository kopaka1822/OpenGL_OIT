// ForwardRenderer.cpp : Defines the entry point for the console application.
//

#include <exception>
#include <iostream>
#include "Framework/Application.h"
#include "ScriptEngine/ScriptEngine.h"
#include <conio.h>
#include "Framework/Profiler.h"
#include "Framework/AsynchInput.h"
#include "Dependencies/gl/buffer.hpp"
#include "Dependencies/gl/texture.hpp"
#include "Framework/DebugContext.h"
#include "Graphics/HotReloadShader.h"

int main(int argc, char** argv)
{
	try
	{
		// bools will be printed as true/false instead of 1/0
		std::cout << std::boolalpha;

		AsynchInput::init();
		ScriptEngine::init();
		Profiler::init();
		HotReloadShader::initScripts();
		Application app;

	
		// run startup script?
		try
		{
			if (argc >= 2)
				ScriptEngine::executeCommand((std::string("execute(\"") + argv[1]) + "\")");
		}
		catch (const std::exception& e)
		{
			std::cerr << "script: " << e.what() << std::endl;
		}

		// main action
		std::string command = "";

		while (app.isRunning())
		{
			app.tick();
			ScriptEngine::iteration();
			HotReloadShader::update();

			command = AsynchInput::get();
			if(!command.empty())
			{
				try
				{
					ScriptEngine::executeCommand(command);
				}
				catch (const std::exception& e)
				{
					std::cerr << "ERR script: " << e.what() << '\n';
				}
			}
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << "ERROR: " << e.what();
		return -1;
	}
    return 0;
}

