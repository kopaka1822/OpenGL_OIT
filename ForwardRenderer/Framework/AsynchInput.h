#pragma once
#include <string>
#include <iostream>
#include <future>

class AsynchInput
{
	static std::string readConsole();
public:
	AsynchInput();

	/// \brief checks if new input is available
	/// \return empty string or input if available
	std::string get()
	{
		std::string res;
		
		if (m_future.wait_for(std::chrono::nanoseconds(0)) == std::future_status::ready)
		{
			res = m_future.get();
			start();
		}
		return res;
	}

	static void setKeywords(std::vector<std::string> keywords);
private:
	void start()
	{
		m_future = std::async(readConsole);
	}

private:
	std::future<std::string> m_future;
};
