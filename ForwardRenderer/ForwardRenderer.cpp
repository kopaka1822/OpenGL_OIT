// ForwardRenderer.cpp : Defines the entry point for the console application.
//

#include <exception>
#include <iostream>
#include "Framework/Application.h"
#include "ScriptEngine/ScriptEngine.h"
#include <conio.h>
#include "Framework/Profiler.h"

int main(int argc, char** argv)
{
	try
	{
		ScriptEngine::init();
		Profiler::init();
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

			while(_kbhit())
			{
				char c = _getch();
				if(c == '\r') // return pressed
				{
					std::cout << std::endl;
					try
					{
						ScriptEngine::executeCommand(command);
					}
					catch(const std::exception& e)
					{
						std::cerr << "script: " << e.what() << std::endl;
					}
					command = "";
				}
				else if(c == '\b')
				{
					if(command.length())
					{
						command.pop_back();
						// erase last character
						std::cout << "\b \b";
					}
				}
				else
				{
					command += c;
					std::cout << c;
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

