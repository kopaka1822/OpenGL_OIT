#pragma once
#include <functional>
#include <vector>
#include "Token.h"

class ScriptEngine
{
public:
	using SetterT = std::function<void(std::vector<Token>)>;
	using GetterT = std::function<std::string(void)>;

	static void init();

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

	static void removeProperty(const std::string& str);

	static void removeFunction(const std::string& str);

	// executes a single command
	static void executeCommand(const std::string& command);
	
	static void executeCommand(const std::string& prefix, const std::string& command);
	// increases the iteration count
	static void iteration();

	static size_t getIteration();
};
