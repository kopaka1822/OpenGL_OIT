#pragma once
#include <glm/detail/type_vec3.hpp>
#include <vector>

class ICamera;
class ITransforms;
class IModel;

class IShadows
{
public:
	virtual ~IShadows() = default;

	struct PointLight
	{
		glm::vec3 position;
	};

	struct DirectionalLight
	{
		glm::vec3 direction;
	};

	// computes and uploads shadows based the passed lights and scene
	virtual void update(const std::vector<PointLight>& pointLights,
		const std::vector<DirectionalLight>& dirLights,
		const IModel& model, ITransforms& transforms,
		const ICamera& cam) = 0;
	// binds shadows
	virtual void bind() const = 0;
};
