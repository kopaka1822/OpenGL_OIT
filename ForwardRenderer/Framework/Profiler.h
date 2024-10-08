#pragma once
#include <string>

class Profiler
{
	Profiler() = default;
public:
	struct Profile
	{
		double min = 0.0;
		double max = 0.0;
		double latest = 0.0;
		double average = 0.0;
		double median = 0.0;

		Profile& operator+=(const Profile& rhs)
		{
			min += rhs.min;
			max += rhs.max;
			latest += rhs.latest;
			average += rhs.average;
			median += rhs.median;
			return *this;
		}
		Profile operator+(const Profile& rhs) const
		{
			auto r(*this);
			return r += rhs;
		}
		double get(const std::string& name) const
		{
			if (name == "min")
				return min;
			if (name == "max")
				return max;
			if (name == "average")
				return average;
			if (name == "median")
				return median;
			return latest;
		}
	};

	static void init();
	// resets all timers
	static void reset();
	static void set(const std::string& name, Profile time);
	static double get(const std::string& name);
	static std::tuple<std::string, double> getActive();
};
