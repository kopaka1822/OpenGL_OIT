#pragma once
#include <glm/matrix.hpp>

class ICamera
{
public:
	virtual ~ICamera(){}
	virtual const glm::mat4& getProjection() const = 0;
	virtual const glm::vec3& getPosition() const = 0;
};