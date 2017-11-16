#pragma once
#include "Graphics/IMaterial.h"
#include <unordered_map>

class SimpleMaterial : public IMaterial
{
	std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_cachedTex;
	std::unordered_map<std::string, glm::vec4> m_cachedAttr;
public:
	void addAttribute(const std::string& name, const glm::vec4& value)
	{
		m_cachedAttr[name] = value;
	}

	void addTexture(const std::string& name, std::shared_ptr<Texture2D> tex)
	{
		m_cachedTex[name] = tex;
	}

	const glm::vec4* getAttribute(const std::string& name) const override
	{
		const auto it = m_cachedAttr.find(name);
		if (it != m_cachedAttr.end())
			return &(it->second);
		return nullptr;
	}
	const Texture2D* getTexture(const std::string& name) const override
	{
		const auto it = m_cachedTex.find(name);
		if (it != m_cachedTex.end())
			return it->second.get();
		return nullptr;
	}
};
