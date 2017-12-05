#include "ICamera.h"
#include "../ScriptEngine/ScriptEngine.h"
#include <iostream>

glm::vec3 ICamera::s_position = glm::vec3(-1.0f, 0.0f, 0.0f);
glm::vec3 ICamera::s_direction = glm::vec3(1.0f, 0.0f, 0.0f);
float ICamera::s_fov = 40.0f;
float ICamera::s_speed = 0.1f;

void ICamera::initScripts()
{
	static bool bInit = false;
	if (bInit) return;
	bInit = true;

	ScriptEngine::addProperty("camSpeed", []()
	{
		std::cout << "speed: " << s_speed << std::endl;
	}, [](std::vector<Token> args)
	{
		if (args.size() == 0)
			throw std::runtime_error("please provide speed");
		if (args[0].getType() != Token::Type::Number)
			throw std::runtime_error("please provide speed as number");
		s_speed = args[0].getFloat();
	});

	ScriptEngine::addProperty("camFov", []()
	{
		std::cout << "fov: " << s_fov << std::endl;
	}, [](std::vector<Token> args)
	{
		if (args.size() == 0)
			throw std::runtime_error("please provide fov degree");
		if (args[0].getType() != Token::Type::Number)
			throw std::runtime_error("please provide fov degree");
		s_fov = args[0].getFloat();
	});

	ScriptEngine::addProperty("camPos", []()
	{
		std::cout << "position: " << 
			s_position.x << ", " << 
			s_position.y << ", " <<
			s_position.z << std::endl;
	}, [](std::vector<Token> args)
	{
		if (args.size() != 3)
			throw std::runtime_error("please provide 3 numbers");
		if (args[0].getType() != Token::Type::Number
			|| args[1].getType() != Token::Type::Number
			|| args[2].getType() != Token::Type::Number)
			throw std::runtime_error("please provide 3 numbers");
		s_position.x = args[0].getFloat();
		s_position.y = args[1].getFloat();
		s_position.z = args[2].getFloat();
	});

	ScriptEngine::addProperty("camDir", []()
	{
		std::cout << "direction: " <<
			s_direction.x << ", " <<
			s_direction.y << ", " <<
			s_direction.z << std::endl;
	}, [](std::vector<Token> args)
	{
		if (args.size() != 3)
			throw std::runtime_error("please provide 3 numbers");
		if (args[0].getType() != Token::Type::Number
			|| args[1].getType() != Token::Type::Number
			|| args[2].getType() != Token::Type::Number)
			throw std::runtime_error("please provide 3 numbers");
		glm::vec3 dir;
		dir.x = args[0].getFloat();
		dir.y = args[1].getFloat();
		dir.z = args[2].getFloat();
		dir = glm::normalize(dir);
		if (glm::dot(dir, dir) == 0.0f)
			throw std::runtime_error("direction must be longer than 0");
		s_direction = dir;
	});
}
