#pragma once
#include "../Graphics/ICamera.h"
#include <glm/gtc/matrix_transform.inl>

class EnvmapCamera : public ICamera
{
public:
	EnvmapCamera(const glm::vec3& pos)
		:
	m_position(pos)
	{}

	void rotateForFace(int faceId)
	{
		m_projection = glm::perspective(1.57079632679f, 1.0f, s_nearPlane, s_farPlane)
			* glm::lookAt(m_position, m_position + getFaceDirection(faceId), getFaceUpVector(faceId));
	}
	
	const glm::mat4& getProjection() const override
	{
		return m_projection;
	}

	const glm::vec3& getPosition() const override
	{
		return m_position;
	}

private:
	static glm::vec3 getFaceDirection(int faceId)
	{
		switch (faceId)
		{
		case 0:
			// right
			return glm::vec3(1.0f, 0.0f, 0.0f);
		case 1:
			// left
			return glm::vec3(-1.0f, 0.0f, 0.0f);
		case 2:
			// up
			return glm::vec3(0.0f, 1.0f, 0.0f);
		case 3:
			// down
			return glm::vec3(0.0f, -1.0f, 0.0f);
		case 4:
			// front
			return glm::vec3(0.0f, 0.0f, 1.0f);
		case 5:
			// back 
			return glm::vec3(0.0f, 0.0f, -1.0f);
		}
		return glm::vec3(0);
	}

	static glm::vec3 getFaceUpVector(int faceId)
	{
		switch (faceId)
		{
		case 0:
		case 1:
		case 4:
		case 5:
			return glm::vec3(0.0f, 1.0f, 0.0f);
		case 2:
			// up
			return glm::vec3(0.0f, 0.0f, -1.0f);
		case 3:
			return glm::vec3(0.0f, 0.0f, 1.0f);
		}
		return glm::vec3(0.0f);
	}

private:
	glm::vec3 m_position;
	glm::mat4 m_projection;
};
