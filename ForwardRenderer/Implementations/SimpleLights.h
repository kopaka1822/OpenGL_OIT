#pragma once
#include "../Graphics/ILights.h"
#include "../Dependencies/gl/buffer.hpp"

class SimpleLights : public ILights
{
	struct LightData
	{
		// point light (if w = 1.0)
		// directional light (if w = 0.0)
		glm::vec4 position;

		glm::vec4 color;

		// x = linear, y = quadratic
		glm::vec4 attenuation;
	};
public:
	void addLight(ParamSet light) override
	{
		m_lights.push_back(std::move(light));
		m_hasChanges = true;
	}


	void removeLight(int index) override
	{
		if (index < 0 || index >= m_lights.size())
			throw std::out_of_range("light index");

		m_lights.erase(m_lights.begin() + index);
		m_hasChanges = true;
	}

	std::string displayLights() override
	{
		int index = 0;
		return std::accumulate(m_lights.begin(), m_lights.end(), std::string("Lights:"), [&index](std::string left, const ParamSet& light)
		{
			return left + "\nLight " + std::to_string(index++) + "\n" + light.toString();
		});
	}

	void upload() override
	{
		if (!m_hasChanges) return;
		m_hasChanges = false;

		auto numLights = m_lights.size();

		assert(m_lights.size() <= 64);

		auto data = std::make_unique<uint8_t[]>(16 + sizeof(LightData) * numLights);
		
		// set the number of lights
		*reinterpret_cast<int*>(data.get()) = int(m_lights.size());

		auto cur = data.get() + 16;

		for(auto i = m_lights.begin(), end = m_lights.end(); i != end; ++i)
		{
			LightData* d = reinterpret_cast<LightData*>(cur);

			if (i->get<glm::vec4>("position"))
			{
				d->position = *i->get<glm::vec4>("position");
				d->position.w = 1.0f;
			}
			else if (i->get<glm::vec4>("direction"))
			{
				d->position = *i->get<glm::vec4>("direction");
				d->position.w = 0.0f;
			}
			else throw std::runtime_error("direction or position is required for light");

			d->color = i->get("color", glm::vec4(1.0));
			d->attenuation.x = i->get("linearAttenuation", 0.0f);
			d->attenuation.y = i->get("quadraticAttenuation", 0.0075f);

			cur += sizeof(LightData);
		}

		m_uniform = gl::StaticUniformBuffer(GLsizei(16 + sizeof(LightData) * numLights), 1, data.get());
	}

	void bind() const override
	{
		// are lights uploaded
		if(m_uniform.getId())
			m_uniform.bind(2);
	}


	size_t numLights() const override
	{
		return m_lights.size();
	}

	const ParamSet& getLight(size_t index) const override
	{
		return m_lights.at(index);
	}

private:
	std::vector<ParamSet> m_lights;
	gl::StaticUniformBuffer m_uniform;
	bool m_hasChanges = true;
};
