#pragma once
#include <string>

class Token
{
public:
	enum class Type
	{
		Number,
		String,
		Identifier,
		Seperator,
		BracketOpen,
		BracketClose,
		Assign,
		Undefined
	};

	Token(Type type, const std::string& value = "")
		:
	m_value(value),
	m_type(type)
	{}
	float getFloat() const
	{
		return atof(m_value.c_str());
	}
	int getInt() const
	{
		return atoi(m_value.c_str());
	}
	const std::string& getString() const
	{
		return m_value;
	}
	Type getType() const
	{
		return m_type;
	}
private:
	std::string m_value;
	Type m_type;
};