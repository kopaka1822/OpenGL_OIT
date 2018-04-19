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
		return static_cast<float>(atof(m_value.c_str()));
	}
	int getInt() const
	{
		return atoi(m_value.c_str());
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