#pragma once
#include "../Graphics/ICamera.h"
#include <glm/gtc/matrix_transform.hpp>

class OrthographicCamera : public ICamera
{
public:
	OrthographicCamera(float width, float height, float nearPlane, float farPlane, const glm::vec3& position, const glm::vec3& direction, const glm::vec3& up)
		: m_width(width),
		  m_height(height),
		  m_near(nearPlane),
		  m_far(farPlane),
		  m_pos(position),
		  m_direction(direction),
		  m_up(up)
	{
		calcProjection();
	}

	const glm::mat4& getProjection() const override
	{
		return m_projection;
	}

	const glm::vec3& getPosition() const override
	{
		return m_pos;
	}

private:
	void calcProjection()
	{
		m_projection = glm::ortho(-m_width / 2.0f, m_width / 2.0f, -m_height / 2.0f, m_height / 2.0f, m_near, m_far)
		* glm::lookAt(m_pos, m_pos + m_direction, m_up);
	}

private:
	float m_width;
	float m_height;
	float m_near;
	float m_far;
	glm::vec3 m_pos;
	glm::vec3 m_direction;
	glm::vec3 m_up;

	glm::mat4 m_projection;
};
