#include "ScriptEngine.h"
#include <unordered_map>
#include <iostream>
#include <regex>
#include <fstream>

std::unordered_map<std::string, ScriptEngine::SetterT> s_functions;
std::unordered_map<std::string, std::pair<ScriptEngine::GetterT, ScriptEngine::SetterT>> s_properties;

void ScriptEngine::addFunction(const std::string& name, SetterT callback)
{
	if (s_functions.find(name) != s_functions.end())
		std::cerr << "WAR: function " << name << " will be overwrittern" << std::endl;
	s_functions[name] = callback;
}

void ScriptEngine::addProperty(const std::string& name, GetterT get, SetterT set)
{
	if (s_properties.find(name) != s_properties.end())
		std::cerr << "WAR: property " << name << " will be overwrittern" << std::endl;
	s_properties[name] = std::make_pair(get, set);
}

void ScriptEngine::addProperty(const std::string& name, GetterT get)
{
	if (s_properties.find(name) != s_properties.end())
		std::cerr << "WAR: property " << name << " will be overwrittern" << std::endl;
	s_properties[name] = std::make_pair(get, SetterT([name](std::vector<Token>)
	{
		std::cerr << name << " is read only" << std::endl;
	}));
}

static Token makeTokenFromString(const std::string& str)
{
	// this is either a number or identifier token
	static std::regex float_regex("[+-]?([0-9]*[.])?[0-9]+");
	if (std::regex_match(str, float_regex))
		return Token(Token::Type::Number, str);
	return Token(Token::Type::Identifier, str);
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
		Token currentToken = Token(Token::Type::Undefined);
		if (isString)
		{
			if (c == '\"')
			{
				// string end
				isString = false;
				currentToken = Token(Token::Type::String, current);
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
				currentToken = Token(Token::Type::Assign);
				break;
			case ',':
				finishLast = true;
				currentToken = Token(Token::Type::Seperator);
				break;
			case '(':
				finishLast = true;
				currentToken = Token(Token::Type::BracketOpen);
				break;
			case ')':
				finishLast = true;
				currentToken = Token(Token::Type::BracketClose);
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
		if (currentToken.getType() != Token::Type::Undefined)
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
std::vector<Token> getArgumentList(const std::vector<Token>& tokens, int start, Token::Type endType)
{
	std::vector<Token> args;
	
	if (start < tokens.size() && endType != Token::Type::Undefined
		&& tokens[start].getType() == endType)
		return args; // no arguments

	while(start < tokens.size())
	{
		if (tokens[start].getType() != Token::Type::Identifier
			&& tokens[start].getType() != Token::Type::Number
			&& tokens[start].getType() != Token::Type::String)
			throw std::runtime_error("expected number, string or identifier in argument list");
		
		args.push_back(tokens[start]);
		if (start + 1 >= tokens.size())
		{
			if (endType == Token::Type::Undefined)
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
		if (tokens[start + 1].getType() != Token::Type::Seperator)
			throw std::runtime_error("missing , in argument list");
		start += 2;
	}

	return args;
}

void ScriptEngine::executeCommand(const std::string& command)
{
	auto tokens = getTokens(command);

	// determine which kind of command
	if (tokens.size() == 0)
		return;

	if (tokens[0].getType() != Token::Type::Identifier)
		throw std::runtime_error("expected idetifier");

	// function, get or set?
	if(tokens.size() == 1)
	{
		// this is a getter
		auto it = s_properties.find(tokens[0].getString());
		if (it == s_properties.end())
			throw std::runtime_error("cannot find property " + tokens[0].getString());
		it->second.first();
		return;
	}
	if(tokens[1].getType() == Token::Type::Assign)
	{
		// this is a property
		auto it = s_properties.find(tokens[0].getString());
		if (it == s_properties.end())
			throw std::runtime_error("cannot find property " + tokens[0].getString());

		it->second.second(getArgumentList(tokens, 2, Token::Type::Undefined));

		return;
	}
	if(tokens[1].getType() == Token::Type::BracketOpen)
	{
		// this is a function
		auto it = s_functions.find(tokens[0].getString());
		if (it == s_functions.end())
			throw std::runtime_error("cannot find property " + tokens[0].getString());

		it->second(getArgumentList(tokens, 2, Token::Type::BracketClose));

		return;
	}
	
	throw std::runtime_error("expected '=' or '(' but found: " + tokens[1].getString());
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
				if (line.length() > 1) // assume it is some kind of command
					std::cout << linecount << ": " << line << std::endl;
				ScriptEngine::executeCommand(line);
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
	});

	addProperty("help", []()
	{
		// list everything
		std::cout << "functions: " << '\n';
		for (const auto& f : s_functions)
			std::cout << "   " << f.first << "(...)" << '\n';
		std::cout << "properties: " << '\n';
		for (const auto& p : s_properties)
			std::cout << "   " << p.first << '\n';
	});
}
