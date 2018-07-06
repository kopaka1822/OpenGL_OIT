#pragma once
#include <functional>
#include <vector>
#include "Token.h"

class ScriptEngine
{
public:
	using FunctionT = std::function<std::string(std::vector<Token>)>;
	using SetterT = std::function<void(std::vector<Token>)>;
	using GetterT = std::function<std::string(void)>;

	static void init();

	static void addFunction(
		const std::string& name,
		FunctionT callback
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

	// keyword which is used for console auto complete
	static void addKeyword(const std::string& name);

	static void removeProperty(const std::string& str);

	static void removeFunction(const std::string& str);

	// executes a single command
	static void executeCommand(const std::string& command);
	
	static void executeCommand(const std::string& prefix, const std::string& command);
	// increases the iteration count
	static void iteration();

	static size_t getIteration();
	static size_t getWaitIteration();
};
