#include "ScriptEngine.h"
#include <unordered_map>
#include <iostream>
#include <regex>
#include <fstream>
#include <queue>
#include <cassert>
#include <numeric>

static std::unordered_map<std::string, ScriptEngine::FunctionT> s_functions;
static std::unordered_map<std::string, std::pair<ScriptEngine::GetterT, ScriptEngine::SetterT>> s_properties;
static size_t s_curIteration = 0;
static size_t s_waitIterations = 0;
static std::queue<std::pair<std::string, std::string>> s_commandQueue;
static std::unordered_map<std::string, Token> s_variables;

void ScriptEngine::addFunction(const std::string& name, FunctionT callback)
{
	if (s_functions.find(name) != s_functions.end())
		std::cerr << "WAR: function " << name << " will be overwrittern\n";
	s_functions[name] = callback;
}

void ScriptEngine::addProperty(const std::string& name, GetterT get, SetterT set)
{
	if (s_properties.find(name) != s_properties.end())
		std::cerr << "WAR: property " << name << " will be overwrittern\n";
	s_properties[name] = std::make_pair(get, set);
}

void ScriptEngine::addProperty(const std::string& name, GetterT get)
{
	if (s_properties.find(name) != s_properties.end())
		std::cerr << "WAR: property " << name << " will be overwrittern\n";
	s_properties[name] = std::make_pair(get, SetterT([name](std::vector<Token>)
	{
		std::cerr << name << " is read only" << std::endl;
	}));
}

void ScriptEngine::removeProperty(const std::string& str)
{
	auto it = s_properties.find(str);
	if (it == s_properties.end())
	{
		std::cerr << "WAR: property " << str << " cannot be erased (not found)\n";
		return;
	}

	s_properties.erase(it);
}

void ScriptEngine::removeFunction(const std::string& str)
{
	auto it = s_functions.find(str);
	if (it == s_functions.end())
	{
		std::cerr << "WAR: function " << str << " cannot be erased (not found)\n";
		return;
	}

	s_functions.erase(it);
}

static Token makeTokenFromString(const std::string& str)
{
	// this is either a number, variable or identifier token
	static std::regex float_regex("[+-]?([0-9]*[.])?[0-9]+");
	if (std::regex_match(str, float_regex))
		return Token(Token::Type::NUMBER, str);
	return Token(Token::Type::IDENTIFIER, str);
}

std::vector<Token> getTokens(const std::string& command)
{
	std::vector<Token> tokens;
	tokens.reserve(100);

	// parse tokens
	bool isString = false;
	std::string current = "";
	for (const auto& c : command)
	{
		bool finishLast = false;
		Token currentToken = Token(Token::Type::UNDEFINED);
		if (isString)
		{
			if (c == '\"')
			{
				// string end
				isString = false;
				currentToken = Token(Token::Type::STRING, current);
				current = "";
			}
			// append character
			else current += c;
		}
		else
		{
			// no string
			switch (c)
			{
			case '\"':
				isString = true;
				finishLast = true;
				break;
			case '=':
				finishLast = true;
				currentToken = Token(Token::Type::ASSIGN);
				break;
			case ',':
				finishLast = true;
				currentToken = Token(Token::Type::SEPERATOR);
				break;
			case '(':
				finishLast = true;
				currentToken = Token(Token::Type::BRACKET_OPEN);
				break;
			case ')':
				finishLast = true;
				currentToken = Token(Token::Type::BRACKET_CLOSE);
				break;
			case '>':
				finishLast = true;
				currentToken = Token(Token::Type::VARIABLE);
				break;
			default:
				if (isspace(static_cast<unsigned char>(c)))
				{
					finishLast = true;
				}
				else current += c;
			}
		}

		if (finishLast && current.length())
		{
			// make token from string
			tokens.push_back(makeTokenFromString(current));
			current = "";
		}
		if (currentToken.getType() != Token::Type::UNDEFINED)
			tokens.push_back(currentToken);
	}

	if (current.length())
	{
		// make token from string
		tokens.push_back(makeTokenFromString(current));
	}

	return tokens;
}


/**
 * \brief 
 * \param tokens tokens to verify
 * \param start start index in token array
 * \param endType token type which should be the last element (e.g. bracket close).
 * undefined if no end type is required (property)
 * \return 
 */
std::vector<Token> getArgumentList(const std::vector<Token>& tokens, size_t start, Token::Type endType)
{
	std::vector<Token> args;
	
	if (start < tokens.size() && endType != Token::Type::UNDEFINED
		&& tokens[start].getType() == endType)
		return args; // no arguments

	while(start < tokens.size())
	{
		if (tokens[start].getType() != Token::Type::IDENTIFIER
			&& tokens[start].getType() != Token::Type::NUMBER
			&& tokens[start].getType() != Token::Type::STRING
			&& tokens[start].getType() != Token::Type::VARIABLE)
			throw std::runtime_error("expected number, string, variable or identifier in argument list");
		
		if(tokens[start].getType() == Token::Type::VARIABLE)
		{
			// fetch variable value
			if (start + 1 == tokens.size() || tokens[start + 1].getType() != Token::Type::IDENTIFIER)
				throw std::runtime_error("expected variable identifier");
			++start;

			// get variable value
			const auto it = s_variables.find(tokens[start].getString());
			if (it == s_variables.end())
				args.emplace_back(Token::Type::STRING);
			else
				args.push_back(it->second);
		}
		else
			args.push_back(tokens[start]);
		
		// next element
		if (start + 1 >= tokens.size())
		{
			if (endType == Token::Type::UNDEFINED)
				break;
			throw std::runtime_error("argument list was not closed appropriately");
		}
		// last element=
		if(start + 1 == tokens.size() - 1)
		{
			if (tokens[start + 1].getType() == endType)
				break;
			throw std::runtime_error("argument list was not closed appropriately");
		}
		// another element?
		if (tokens[start + 1].getType() != Token::Type::SEPERATOR)
			throw std::runtime_error("missing , in argument list");
		start += 2;
	}

	return args;
}

static void s_executeCommand(const std::string& command, const std::string* const prefix)
{
	auto tokens = getTokens(command);

	// determine which kind of command
	if (tokens.empty())
		return;

	if (tokens[0].getType() != Token::Type::IDENTIFIER
		&& tokens[0].getType() != Token::Type::VARIABLE)
		throw std::runtime_error("expected identifier or variable");

	if (s_waitIterations)
	{
		// just enqueue command
		s_commandQueue.push(std::make_pair(prefix ? (*prefix) : ("script "), command));
		return;
	}
	if (prefix)
		std::cout << *prefix << " " << command << '\n';

	// function, get or set?
	if (tokens.size() == 1 && tokens[0].getType() == Token::Type::IDENTIFIER)
	{
		// this is a getter
		const auto it = s_properties.find(tokens[0].getString());
		if (it == s_properties.end())
			throw std::runtime_error("cannot find property " + tokens[0].getString());
		std::cout << tokens[0].getString() << ": " << it->second.first() << '\n';
		return;
	}
	if(tokens.size() >= 2)
	{
		if (tokens[0].getType() == Token::Type::IDENTIFIER && tokens[1].getType() == Token::Type::ASSIGN)
		{
			// this is a property setter
			auto it = s_properties.find(tokens[0].getString());
			if (it == s_properties.end())
				throw std::runtime_error("cannot find property " + tokens[0].getString());

			it->second.second(getArgumentList(tokens, 2, Token::Type::UNDEFINED));

			return;
		}
		if (tokens[0].getType() == Token::Type::IDENTIFIER && tokens[1].getType() == Token::Type::BRACKET_OPEN)
		{
			// this is a function
			const auto it = s_functions.find(tokens[0].getString());
			if (it == s_functions.end())
				throw std::runtime_error("cannot find property " + tokens[0].getString());

			auto res = it->second(getArgumentList(tokens, 2, Token::Type::BRACKET_CLOSE));
			if (res.length())
				std::cout << res << '\n';

			return;
		}
		if (tokens[0].getType() == Token::Type::VARIABLE)
		{
			// variable getter or setter
			if (tokens[1].getType() != Token::Type::IDENTIFIER)
				throw std::runtime_error("expected variable identifier");

			if (tokens.size() == 2)
			{
				// variable getter
				const auto it = s_variables.find(tokens[1].getString());
				if (it == s_variables.end())
					std::cout << tokens[1].getString() << ":\n";
				else
					std::cout << it->first << ": " << it->second.getString() << "\n";
				return;
			}

			// TODO make complex setter?
			if (tokens.size() < 4)
				throw std::runtime_error("expected variable assign (>name = value)");

			// must be variable setter
			if (tokens[2].getType() != Token::Type::ASSIGN)
				throw std::runtime_error("expected variable assign");

			// set variable
			auto args = getArgumentList(tokens, 3, Token::Type::UNDEFINED);
			if (args.size() != 1)
				throw std::runtime_error("expected only one argument for variable setter");
			s_variables[tokens[1].getString()] = args[0];
			return;
		}
	}

	throw std::runtime_error("expected '=' or '(' but found: " + tokens[1].getString());
}

void ScriptEngine::executeCommand(const std::string& command)
{
	s_executeCommand(command, nullptr);
}

void ScriptEngine::executeCommand(const std::string& prefix, const std::string& command)
{
	s_executeCommand(command, &prefix);
}


void ScriptEngine::iteration()
{
	// execute enqueued commands
	while (!s_commandQueue.empty() && !s_waitIterations)
	{
		try
		{
			const auto cmd = s_commandQueue.front();
			s_commandQueue.pop();
			std::cout << cmd.first << " " << cmd.second << '\n';
			executeCommand(cmd.second);
		}
		catch (const std::exception& e)
		{
			std::cerr << "ERR script: " << e.what() << '\n';
		}
	}
	

	++s_curIteration;

	if (s_waitIterations)
		--s_waitIterations;
}

size_t ScriptEngine::getIteration()
{
	return s_curIteration;
}

static void openScriptFile(const std::string& filename)
{
	std::ifstream file;
	file.open(filename);

	std::string output;
	std::string line;
	if (!file.is_open())
		throw std::runtime_error("could not open script file " + filename);

	auto linecount = 1;
	try
	{
		while (file.good())
		{
			std::getline(file, line);

			// commented out?
			if(!(line.size() >= 2 && line[0] == '/' && line[1] == '/'))
			{
				ScriptEngine::executeCommand(filename + " " + std::to_string(linecount) + ":" , line);
			}


			linecount++;
		}
	}
	catch(const std::exception& e)
	{
		throw std::runtime_error("error in file " + filename + " line " + std::to_string(linecount)
			+ std::string(": ") + e.what());
	}
}

void ScriptEngine::init()
{
	addFunction("execute", 
		[](std::vector<Token> tokens)
	{
		for (const auto& t : tokens)
			openScriptFile(t.getString());
		return "";
	});

	addFunction("waitIterations", [](const std::vector<Token> tokens)
	{
		if (tokens.empty())
			throw std::runtime_error("expected iterations");

		s_waitIterations = size_t(tokens.at(0).getInt());
		return "";
	});

	addProperty("help", []()
	{
		// list everything
		std::string res = "functions:\n";

		std::vector<std::string> names;
		names.reserve(std::max(s_functions.size(), s_properties.size()));
		for (const auto& f : s_functions)
			names.push_back(f.first);

		// sort names
		std::sort(names.begin(), names.end());

		for (const auto& n : names)
			res += "   " + n + "(...)\n";
		
		res += "properties:\n";
		
		names.resize(0);
		for (const auto& p : s_properties)
			names.push_back(p.first);

		// sort names
		std::sort(names.begin(), names.end());
		
		for (const auto& n : names)
			res += "   " + n + "\n";

		return res;
	});

	// simple algebraic operations
	addFunction("add", [](const std::vector<Token>& args)
	{
		if (args.at(0).isInt() && args.at(1).isInt())
			// integer addition
			return std::to_string(args.at(0).getInt() + args.at(1).getInt());

		// assume floating point
		return std::to_string(args.at(0).getFloat() + args.at(1).getFloat());
	});
	addFunction("subtract", [](const std::vector<Token>& args)
	{
		if (args.at(0).isInt() && args.at(1).isInt())
			// integer addition
			return std::to_string(args.at(0).getInt() - args.at(1).getInt());

		// assume floating point
		return std::to_string(args.at(0).getFloat() - args.at(1).getFloat());
	});
	addFunction("divide", [](const std::vector<Token>& args)
	{
		// assume floating point
		return std::to_string(args.at(0).getFloat() / args.at(1).getFloat());
	});
	addFunction("multiply", [](const std::vector<Token>& args)
	{
		if (args.at(0).isInt() && args.at(1).isInt())
			// integer addition
			return std::to_string(args.at(0).getInt() * args.at(1).getInt());

		// assume floating point
		return std::to_string(args.at(0).getFloat() * args.at(1).getFloat());
	});

	// integer conversion
	addFunction("int", [](const std::vector<Token>& args)
	{
		return std::to_string(args.at(0).getInt());
	});

	// string concatenation
	addFunction("concat", [](const std::vector<Token>& args)
	{
		return std::accumulate(args.begin(), args.end(), std::string(), [](const auto& prev, const auto& token)
		{
			return prev + token.getString();
		});
	});
}
