#pragma once
#include <string>
#include <iostream>
#include <future>

class AsynchInput
{
	AsynchInput() = default;
public:
	/// \brief checks if new input is available
	/// \return empty string or input if available
	static std::string get();
	/// \brief acquired windows console handles
	static void init();
	static void setKeywords(std::vector<std::string> keywords);
};
