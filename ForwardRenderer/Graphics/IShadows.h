#pragma once
#include <glm/detail/type_vec3.hpp>
#include <vector>
#include "../Implementations/OrthographicCamera.h"

class ICamera;
class ITransforms;
class IModel;

class IShadows
{
public:
	virtual ~IShadows() = default;

	struct PointLight
	{
		explicit PointLight(const glm::vec3& position)
			: position(position)
		{
		}

		glm::vec3 position;
	};

	struct DirectionalLight
	{
		DirectionalLight(const glm::vec3& direction, const OrthographicCamera& camera)
			: direction(direction),
			  camera(camera)
		{
		}

		glm::vec3 direction;
		// camera from the lights perspective
		OrthographicCamera camera;
	};

	// computes and uploads shadows based the passed lights and scene
	virtual void update(
		const std::vector<PointLight>& pointLights,
		const std::vector<DirectionalLight>& dirLights,
		const IModel& model, ITransforms& transforms) = 0;
	// binds shadows
	virtual void bind() const = 0;
};
