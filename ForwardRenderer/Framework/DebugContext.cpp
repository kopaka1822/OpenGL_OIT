#include "DebugContext.h"
#include <exception>
#include <glad/glad.h>
#include <iostream>
#include <string>
#include "../ScriptEngine/ScriptEngine.h"

static bool s_detailedOutput = true;

static void glDebugOutput(GLenum _source, GLenum _type, GLuint _id, GLenum _severity, GLsizei _length, const GLchar* _message, const void* _userParam)
{
	std::string debSource, debType, debSev;

	if (_source == GL_DEBUG_SOURCE_API)
		debSource = "OpenGL";
	else if (_source == GL_DEBUG_SOURCE_WINDOW_SYSTEM)
		debSource = "Windows";
	else if (_source == GL_DEBUG_SOURCE_SHADER_COMPILER)
		debSource = "Shader Compiler";
	else if (_source == GL_DEBUG_SOURCE_THIRD_PARTY)
		debSource = "Third Party";
	else if (_source == GL_DEBUG_SOURCE_APPLICATION)
		debSource = "Application";
	else if (_source == GL_DEBUG_SOURCE_OTHER)
		debSource = "Other";

	if (_type == GL_DEBUG_TYPE_ERROR)
		debType = "error";
	else if (_type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
		debType = "deprecated behavior";
	else if (_type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
		debType = "undefined behavior";
	else if (_type == GL_DEBUG_TYPE_PORTABILITY)
		debType = "portability";
	else if (_type == GL_DEBUG_TYPE_PERFORMANCE)
		debType = "performance";
	else if (_type == GL_DEBUG_TYPE_OTHER)
		debType = "message";

	if (_severity == GL_DEBUG_SEVERITY_HIGH)
		debSev = "high";
	else if (_severity == GL_DEBUG_SEVERITY_MEDIUM)
		debSev = "medium";
	else if (_severity == GL_DEBUG_SEVERITY_LOW)
		debSev = "low";
	else if (_severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		debSev = "note";

	std::string logMessage = debSource + ": " + debType + "(" + debSev + ") " + std::to_string(_id) + ": " + _message;
	if (_type == GL_DEBUG_TYPE_ERROR || _type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
		throw std::exception(logMessage.c_str());
	
	if (!s_detailedOutput)
		return;

	if (_type == GL_DEBUG_TYPE_PERFORMANCE)
		std::cerr << "INF: " << logMessage.c_str() << '\n';
	else
		std::cerr << "WAR: " << logMessage.c_str() << '\n';
}

void DebugContext::Init()
{
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_TRUE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glDebugOutput, nullptr);

	ScriptEngine::addProperty("debugOutput", []()
	{
		return std::to_string(s_detailedOutput);
	}, [](const std::vector<Token>& args)
	{
		s_detailedOutput = args.at(0).getString() != "false";
		std::cout << (s_detailedOutput ? "enabled" : "disabled") << " debug output\n";
	});
}
