#pragma once
#include "Graphics/ILights.h"
#include "Dependencies/gl/buffer.hpp"

class SimpleLights : public ILights
{
	struct LightData
	{
		// point light (if w = 1.0)
		// directional light (if w = 0.0)
		glm::vec4 position;

		glm::vec3 color;
		float quadAttenuation;

		float linearAttenuation;
		glm::vec3 padding;
	};
public:
	void addLight(ParamSet light) override
	{
		m_lights.push_back(std::move(light));
	}

	void upload() override
	{
		std::vector<LightData> data;
		data.resize(m_lights.size());

		std::transform(m_lights.begin(), m_lights.end(), data.begin(), [](const ParamSet& l)
		{
			LightData d;

			if(l.get<glm::vec4>("position"))
			{
				d.position = *l.get<glm::vec4>("position");
				d.position.w = 1.0f;
			}
			else if(l.get<glm::vec4>("direction"))
			{
				d.position = *l.get<glm::vec4>("direction");
				d.position.w = 0.0f;
			}
			else throw std::runtime_error("direction or position is required for light");

			d.color = l.get("color", glm::vec3(1.0));
			d.linearAttenuation = l.get("linearAttenuation", 0.0f);
			d.quadAttenuation = l.get("quadraticAttenuation", 0.0075f);

			return d;
		});

		m_uniform = gl::StaticUniformBuffer(data);
	}

	void bind() const override
	{
		// are lights uploaded
		if(m_uniform.getId())
			m_uniform.bind(2);
	}

private:
	std::vector<ParamSet> m_lights;
	gl::StaticUniformBuffer m_uniform;
};
