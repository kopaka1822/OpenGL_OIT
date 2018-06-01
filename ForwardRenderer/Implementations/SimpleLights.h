#pragma once
#include "../Graphics/ILights.h"
#include "../Dependencies/gl/buffer.hpp"
#include "OrthographicCamera.h"
#include "../Graphics/IShadows.h"

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

		glm::vec4 padding0;

		// light space matrix for directional lights
		glm::mat4 lightSpaceMatrix;
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

	void upload(IShadows& shadows, const IModel& model, ITransforms& transforms) override
	{
		m_hasChanges |= &shadows != m_lastShadows;
		m_hasChanges |= &model != m_lastModel;

		m_lastShadows = &shadows;
		m_lastModel = &model;

		if (!m_hasChanges) return;

		std::cerr << "uploading lights and shadow maps\n";
		m_hasChanges = false;

		auto numLights = m_lights.size();

		assert(m_lights.size() <= 64);

		auto data = std::make_unique<uint8_t[]>(16 + sizeof(LightData) * numLights);

		std::vector<IShadows::DirectionalLight> dirLights;
		std::vector<IShadows::PointLight> pointLights;
		updateBBox(model);

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
				pointLights.emplace_back(d->position);
			}
			else if (i->get<glm::vec4>("direction"))
			{
				d->position = *i->get<glm::vec4>("direction");
				d->position.w = 0.0f;

				auto cam = calculateDirectionalMatrix(*d);
				d->lightSpaceMatrix = cam.getProjection();

				dirLights.emplace_back(d->position, cam);
			}
			else throw std::runtime_error("direction or position is required for light");

			d->color = i->get("color", glm::vec4(1.0));
			d->attenuation.x = i->get("linearAttenuation", 0.0f);
			d->attenuation.y = i->get("quadraticAttenuation", 0.0075f);

			cur += sizeof(LightData);
		}

		m_uniform = gl::StaticUniformBuffer(GLsizei(16 + sizeof(LightData) * numLights), 1, data.get());

		// update shadows
		shadows.update(pointLights, dirLights, model, transforms);
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
	void updateBBox(const IModel& model)
	{
		// combute bounding box
		m_bboxCenter = (model.getBoundingMin() + model.getBoundingMax()) / 2.0f;

		auto min = model.getBoundingMin();
		auto max = model.getBoundingMax();

		m_bboxEdges[0] = min;

		m_bboxEdges[1] = min;
		m_bboxEdges[1].x = max.x;

		m_bboxEdges[2] = min;
		m_bboxEdges[2].y = max.y;

		m_bboxEdges[3] = min;
		m_bboxEdges[3].z = max.z;

		m_bboxEdges[4] = min;
		m_bboxEdges[4].x = max.x;
		m_bboxEdges[4].y = max.y;

		m_bboxEdges[5] = min;
		m_bboxEdges[5].x = max.x;
		m_bboxEdges[5].z = max.z;

		m_bboxEdges[6] = min;
		m_bboxEdges[6].y = max.y;
		m_bboxEdges[6].z = max.z;

		m_bboxEdges[7] = max;
	}

	OrthographicCamera calculateDirectionalMatrix(const LightData& d)
	{
		auto direction = glm::normalize(glm::vec3(d.position));

		float nearPlaneDistance = 0.0f;
		// determine look at
		for (const auto& edge : m_bboxEdges)
		{
			nearPlaneDistance = glm::max(nearPlaneDistance, glm::dot(-direction, edge - m_bboxCenter));
		}

		auto origin = nearPlaneDistance * -direction + m_bboxCenter;

		auto up = abs(dot(glm::vec3(0.0, 1.0f, 0.0f), direction)) < abs(dot(glm::vec3(0.0f, 0.0f, 1.0f), direction)) 
		? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
		auto right = glm::normalize(glm::cross(up, direction));

		float farPlane = 0.0f;
		float halfWidth = 0.0f;
		float halfHeight = 0.0f;
		for (const auto& edge : m_bboxEdges)
		{
			farPlane = glm::max(farPlane, glm::dot(direction, edge - origin));
			halfWidth = glm::max(halfWidth, glm::dot(right, edge - origin));
			halfHeight = glm::max(halfHeight, glm::dot(up, edge - origin));
		}

		auto lightCam = OrthographicCamera(halfWidth * 2, halfHeight * 2, 0.0f, farPlane, origin, m_bboxCenter, up);

		return lightCam;
	}

private:
	std::vector<ParamSet> m_lights;
	gl::StaticUniformBuffer m_uniform;
	bool m_hasChanges = true;

	// model bounding box
	glm::vec3 m_bboxCenter;
	std::array<glm::vec3, 8> m_bboxEdges;

	// update information
	const IModel* m_lastModel = nullptr;
	const IShadows* m_lastShadows = nullptr;
};
