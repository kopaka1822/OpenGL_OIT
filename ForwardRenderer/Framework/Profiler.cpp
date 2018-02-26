#include "Profiler.h"
#include "../ScriptEngine/ScriptEngine.h"
#include <unordered_map>
#include <iostream>

static std::unordered_map<std::string, Profiler::Profile> m_profiles;
static std::string m_activeProfile = "time";
static std::string s_activeType = "latest";

void Profiler::init()
{
	ScriptEngine::addProperty("profiler", []() {std::cout << "profiler: " << m_activeProfile << '\n'; }, [](const std::vector<Token>& args)
	{
		if (args.empty())
			throw std::runtime_error("profiler name missing");
		m_activeProfile = args.at(0).getString();
	});

	ScriptEngine::addProperty("profiles", []()
	{
		for (const auto& p : m_profiles)
			std::cout << "   " << p.first << '\n';
	});
	ScriptEngine::addFunction("getProfileTime", [](const std::vector<Token>& args)
	{
		if(args.empty())
		{
			std::cout << m_activeProfile << ": " << get(m_activeProfile) << "\n";
		}
		else
		{
			std::cout << args.at(0).getString() << ": " << get(args.at(0).getString()) << "\n";
		}
	});
	ScriptEngine::addProperty("profileType", []() {std::cout << "profileType: " << s_activeType << '\n'; }, [](const std::vector<Token>& args)
	{
		if (args.empty())
			throw std::runtime_error("profile type missing. Use min, max, average or latest");
		auto val = args[0].getString();
		if (val == "min" || val == "max" || val == "latest" || val == "average")
			s_activeType = val;
		else std::cerr << "type must be min, max, average or latest\n";
	});
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
	return { m_activeProfile, get(m_activeProfile) };
}
