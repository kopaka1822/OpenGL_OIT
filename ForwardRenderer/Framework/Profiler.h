#pragma once
#include <string>

class Profiler
{
	Profiler() = default;
public:
	static void init();
	static void set(const std::string& name, double time);
	static double get(const std::string& name);
	static std::tuple<std::string, double> getActive();
};
