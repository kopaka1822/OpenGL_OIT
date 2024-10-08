#pragma once
#include <unordered_map>
#include <glm/glm.hpp>
#include <memory>
#include "../Graphics/CachedTexture2D.h"


class ParamSet
{
	std::unordered_map<std::string, std::shared_ptr<CachedTexture2D>> m_cachedTex;
	std::unordered_map<std::string, glm::vec4> m_params;
	std::unordered_map<std::string, std::string> m_stringParams;
public:
	ParamSet() = default;
	ParamSet(ParamSet&&) = default;
	ParamSet& operator=(ParamSet&&) = default;

	// add functions

	void add(const std::string& name, const glm::vec4& value)
	{
		m_params[name] = value;
	}

	void add(const std::string& name, const glm::vec3& value)
	{
		add(name, glm::vec4(value, 0.0f));
	}

	void add(const std::string& name, const glm::vec2& value)
	{
		add(name, glm::vec4(value, 0.0f, 0.0f));
	}

	void add(const std::string& name, float value)
	{
		add(name, glm::vec4(value, 0.0f, 0.0f, 0.0f));
	}

	void add(const std::string& name, std::string value)
	{
		m_stringParams[name] = move(value);
	}

	void addTexture(const std::string& name, std::shared_ptr<CachedTexture2D > tex)
	{
		m_cachedTex[name] = move(tex);
	}

	// get functions

	/**
	* \brief returns the parameter or nullptr if not found
	* \param name of the parameter
	* \return
	*/
	template<class T>
	const T* get(const std::string& name) const = delete;

	template<>
	const std::string* get(const std::string& name) const
	{
		const auto it = m_stringParams.find(name);
		if (it == m_stringParams.end()) return nullptr;
		return &(it->second);
	}

	template<>
	const glm::vec4* get(const std::string& name) const
	{
		const auto it = m_params.find(name);
		if (it == m_params.end()) return nullptr;
		return &(it->second);
	}

	template<>
	const glm::vec3* get(const std::string& name) const
	{
		const auto it = m_params.find(name);
		if (it == m_params.end()) return nullptr;
		return reinterpret_cast<const glm::vec3*>(&(it->second));
	}

	template<>
	const glm::vec2* get(const std::string& name) const
	{
		const auto it = m_params.find(name);
		if (it == m_params.end()) return nullptr;
		return reinterpret_cast<const glm::vec2*>(&(it->second));
	}

	template<>
	const float* get(const std::string& name) const
	{
		const auto it = m_params.find(name);
		if (it == m_params.end()) return nullptr;
		return &(it->second.x);
	}

	/**
	 * \brief 
	 * \param name of the parameter
	 * \param defaultValue value that should be returned if parameter was not found
	 * \return 
	 */
	std::string get(const std::string& name, const std::string& defaultValue) const
	{
		auto val = get<std::string>(name);
		return val ? *val : defaultValue;
	}

	glm::vec4 get(const std::string& name, const glm::vec4& defaultValue) const
	{
		auto val = get<glm::vec4>(name);
		return val ? *val : defaultValue;
	}

	glm::vec3 get(const std::string& name, const glm::vec3& defaultValue) const
	{
		auto val = get<glm::vec3>(name);
		return val ? *val : defaultValue;
	}

	glm::vec2 get(const std::string& name, const glm::vec2& defaultValue) const
	{
		auto val = get<glm::vec2>(name);
		return val ? *val : defaultValue;
	}

	float get(const std::string& name, float defaultValue) const
	{
		auto val = get<float>(name);
		return val ? *val : defaultValue;
	}

	std::shared_ptr<CachedTexture2D> getTexture(const std::string& name) const
	{
		const auto it = m_cachedTex.find(name);
		if (it == m_cachedTex.end()) return nullptr;
		return it->second;
	}

	std::shared_ptr<CachedTexture2D> getTexture(const std::string& name, const std::shared_ptr<CachedTexture2D>& defaultValue) const
	{
		auto val = getTexture(name);
		return val ? val : defaultValue;
	}

	std::string toString() const
	{
		std::string res;
		for(const auto& val : m_stringParams)
		{
			res += val.first + ": " + val.second + "\n";
		}
		for(const auto& val : m_params)
		{
			res += val.first + ": " + std::to_string(val.second.x) + ", " + std::to_string(val.second.y) + ", " + std::to_string(val.second.z) + ", " + std::to_string(val.second.w) + "\n";
		}
		for(const auto& tex : m_cachedTex)
		{
			res += tex.first + ": texture\n";
		}
		return res;
	}

	/**
	 * \brief searches for the corresponding float vector
	 * \param name parameter
	 * \return 
	 */
	bool has(const std::string& name) const
	{
		return m_params.find(name) != m_params.end();
	}

	bool hasTexture(const std::string& name) const
	{
		return m_cachedTex.find(name) != m_cachedTex.end();
	}

	/**
	 * \brief searches for the corresponding string parameter
	 * \param name parameter
	 * \return 
	 */
	bool hasString(const std::string& name) const
	{
		return m_stringParams.find(name) != m_stringParams.end();
	}
};
