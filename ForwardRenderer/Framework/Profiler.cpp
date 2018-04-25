#include "Profiler.h"
#include "../ScriptEngine/ScriptEngine.h"
#include <unordered_map>
#include <iostream>
#include <numeric>

static std::unordered_map<std::string, Profiler::Profile> m_profiles;
static std::string s_activeProfile = "time";
static std::string s_activeType = "latest";

void Profiler::init()
{
	ScriptEngine::addProperty("profiler", []() {return s_activeProfile; }, [](const std::vector<Token>& args)
	{
		if (args.empty())
			throw std::runtime_error("profiler name missing");
		s_activeProfile = args.at(0).getString();
	});

	ScriptEngine::addProperty("profiles", []()
	{
		for (const auto& p : m_profiles)
			std::cout << "   " << p.first << '\n';

		return std::accumulate(m_profiles.begin(), m_profiles.end(), std::string(), [](const auto& prev, const auto& p)
		{
			return prev + "  " + p.first + "\n";
		});
	});
	ScriptEngine::addFunction("getProfileTime", [](const std::vector<Token>& args)
	{
		if(args.empty())
		{
			return std::to_string(get(s_activeProfile));
		}
		else
		{
			return std::to_string(get(args.at(0).getString()));;
		}
	});
	ScriptEngine::addFunction("getProfileTimes", [](const auto& args)
	{
		return std::accumulate(m_profiles.begin(), m_profiles.end(), std::string(), [](const auto& prev, const auto& cur)
		{
			return prev + cur.first + ": " + std::to_string(get(cur.first)) + "\n";
		});
	});
	ScriptEngine::addFunction("resetProfiles", [](const auto&)
	{
		Profiler::reset();
		return "";
	});
	ScriptEngine::addProperty("profileType", []() {return s_activeType; }, [](const std::vector<Token>& args)
	{
		if (args.empty())
			throw std::runtime_error("profile type missing. Use min, max, average or latest");
		auto val = args[0].getString();
		if (val == "min" || val == "max" || val == "latest" || val == "average")
			s_activeType = val;
		else std::cerr << "type must be min, max, average or latest\n";
	});
}

void Profiler::reset()
{
	m_profiles.clear();
}

void Profiler::set(const std::string& name, Profile time)
{
	m_profiles[name] = time;
}

double Profiler::get(const std::string& name)
{
	const auto it = m_profiles.find(name);
	if (it != m_profiles.end())
		return it->second.get(s_activeType);
	return 0.0;
}

std::tuple<std::string, double> Profiler::getActive()
{
	return { s_activeProfile, get(s_activeProfile) };
}
