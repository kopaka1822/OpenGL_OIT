// ForwardRenderer.cpp : Defines the entry point for the console application.
//

#include <exception>
#include <iostream>
#include "Framework/Application.h"

int main()
{
	try
	{
		Application app;
		
		while (app.isRunning())
		{
			app.tick();
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << "ERROR: " << e.what();
	}
    return 0;
}

