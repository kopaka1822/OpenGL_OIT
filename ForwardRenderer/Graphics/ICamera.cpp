#include "ICamera.h"
#include "../ScriptEngine/ScriptEngine.h"
#include <iostream>

glm::vec3 ICamera::s_position = glm::vec3(-1.0f, 0.0f, 0.0f);
glm::vec3 ICamera::s_direction = glm::vec3(1.0f, 0.0f, 0.0f);
float ICamera::s_fov = 40.0f;
float ICamera::s_speed = 0.1f;
float ICamera::s_nearPlane = 0.1f;
float ICamera::s_farPlane = 10000000.0f;

void ICamera::initScripts()
{
	static bool bInit = false;
	if (bInit) return;
	bInit = true;

	ScriptEngine::addProperty("camSpeed", []()
	{
		return std::to_string(s_speed);
	}, [](std::vector<Token> args)
	{
		if (args.size() == 0)
			throw std::runtime_error("please provide speed");
		if (args[0].getType() != Token::Type::NUMBER)
			throw std::runtime_error("please provide speed as number");
		s_speed = args[0].getFloat();
	});

	ScriptEngine::addProperty("camFov", []()
	{
		return std::to_string(s_fov);
	}, [](std::vector<Token> args)
	{
		if (args.size() == 0)
			throw std::runtime_error("please provide fov degree");
		if (args[0].getType() != Token::Type::NUMBER)
			throw std::runtime_error("please provide fov degree");
		s_fov = args[0].getFloat();
	});

	ScriptEngine::addProperty("camNear", []()
	{
		return std::to_string(s_nearPlane);
	}, [](std::vector<Token> args)
	{
		if (args.size() == 0)
			throw std::runtime_error("please provide near plane distance");
		if (args[0].getType() != Token::Type::NUMBER)
			throw std::runtime_error("please provide near plane distance");
		s_nearPlane = args[0].getFloat();
	});

	ScriptEngine::addProperty("camFar", []()
	{
		return std::to_string(s_farPlane);
	}, [](std::vector<Token> args)
	{
		if (args.size() == 0)
			throw std::runtime_error("please provide far plane distance");
		if (args[0].getType() != Token::Type::NUMBER)
			throw std::runtime_error("please provide far plane distance");
		s_farPlane = args[0].getFloat();
	});

	ScriptEngine::addProperty("camPos", []()
	{
		return  
			std::to_string(s_position.x) + ", " + 
			std::to_string(s_position.y) + ", " +
			std::to_string(s_position.z);
	}, [](std::vector<Token> args)
	{
		if (args.size() != 3)
			throw std::runtime_error("please provide 3 numbers");
		if (args[0].getType() != Token::Type::NUMBER
			|| args[1].getType() != Token::Type::NUMBER
			|| args[2].getType() != Token::Type::NUMBER)
			throw std::runtime_error("please provide 3 numbers");
		s_position.x = args[0].getFloat();
		s_position.y = args[1].getFloat();
		s_position.z = args[2].getFloat();
	});

	ScriptEngine::addProperty("camDir", []()
	{
		return
			std::to_string(s_direction.x) + ", " +
			std::to_string(s_direction.y) + ", " +
			std::to_string(s_direction.z);
	}, [](std::vector<Token> args)
	{
		if (args.size() != 3)
			throw std::runtime_error("please provide 3 numbers");
		if (args[0].getType() != Token::Type::NUMBER
			|| args[1].getType() != Token::Type::NUMBER
			|| args[2].getType() != Token::Type::NUMBER)
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
