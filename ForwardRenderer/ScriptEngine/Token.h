#pragma once
#include <string>
#include <regex>

class Token
{
public:
	enum class Type
	{
		NUMBER,
		STRING,
		IDENTIFIER,
		SEPERATOR,
		BRACKET_OPEN,
		BRACKET_CLOSE,
		ASSIGN,
		VARIABLE,
		UNDEFINED
	};

	Token() : m_value(""), m_type(Type::UNDEFINED) {}
	explicit Token(Type type, std::string value = "")
		:
	m_value(move(value)),
	m_type(type)
	{}
	float getFloat() const try
	{
		return std::stof(m_value);
	}
	catch(const std::exception&)
	{
		throw std::runtime_error("cannot convert " + m_value + " to float");
	}

	bool isFloat() const try
	{
		std::stof(m_value);
		return true;
	}
	catch(const std::exception&)
	{
		return false;
	}

	int getInt() const try
	{
		return std::stoi(m_value);
	}
	catch (const std::exception&)
	{
		throw std::runtime_error("cannot convert " + m_value + " to int");
	}

	bool isInt() const try
	{
		const std::regex rgx("[0-9][0-9]*");
		if (!std::regex_match(m_value, rgx))
			return false;

		std::stoi(m_value);
		return true;
	}
	catch(const std::exception&)
	{
		return false;
	}

	const std::string& getString() const
	{
		return m_value;
	}
	
	/**
	 * \brief tries to convert the value to a boolean
	 * \return true if "true", false if "false"
	 * \throws runtime_error if neither true nor false
	 */
	bool getBool() const
	{
		if (m_value == "true")
			return true;
		if (m_value == "false")
			return false;
		throw std::runtime_error("cannot convert " + m_value + " to true or false");
	}
	Type getType() const
	{
		return m_type;
	}
private:
	std::string m_value;
	Type m_type;
};
