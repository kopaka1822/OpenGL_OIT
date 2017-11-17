#pragma once
#include <functional>
#include <vector>
#include "Token.h"

class ScriptEngine
{
public:
	using SetterT = std::function<void(std::vector<Token>)>;
	using GetterT = std::function<void(void)>;

	static void Init();

	static void addFunction(
		const std::string& name,
		SetterT callback
	);
	static void addProperty(
		const std::string& name,
		GetterT get,
		SetterT set
	);
	static void addProperty(
		const std::string& name,
		GetterT get
	);

	// executes a single command
	static void executeCommand(const std::string& command);
};
