#pragma once
#include "../Graphics/ICamera.h"
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../Framework/IMouseReceiver.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "../Framework/IKeyReceiver.h"
#include "../Framework/ITickReveicer.h"
#include <algorithm>
#include "../Framework/IWindowReceiver.h"

class ProjectionCamera : public ICamera, IMouseReceiver, IKeyReceiver, ITickReceiver, IWindowReceiver
{
public:
	ProjectionCamera()
		:
	m_aspect(float(Window::getWidth()) / float(Window::getHeight()))
	{
		calcProjection();
	}

	const glm::mat4& getProjection() const override
	{
		return m_projection;
	}
	const glm::vec3& getPosition() const override
	{
		return s_position;
	}


	~ProjectionCamera() override = default;

	void onMouseMove(double x, double y, double dx, double dy) override
	{
		if(m_mouseDown)
		{
			// rotate
			auto side = glm::normalize(glm::cross(s_direction, glm::vec3(0.0f, -1.0f, 0.0f)));
			auto v = glm::rotate(glm::mat4(), float(dy) * 0.01f, side) * glm::vec4(s_direction, 0.0f);
			v = glm::rotate(glm::mat4(), float(dx) * 0.01f, glm::vec3(0.0f, -1.0f, 0.0f)) * v;

			s_direction = glm::vec3(v.x, v.y, v.z);
			calcProjection();
		}
	}

	void onButtonDown(int button) override
	{
		if (button == GLFW_MOUSE_BUTTON_1)
			m_mouseDown = true;
	}

	void onButtonUp(int button) override
	{
		if (button == GLFW_MOUSE_BUTTON_1)
			m_mouseDown = false;
	}

	void onScroll(double x, double y) override
	{
		// for now mouse speed
		if (y > 0.0)
			s_speed *= static_cast<float>(std::pow(2, y));
		else if (y < 0.0)
			s_speed *= static_cast<float>(1.0 / std::pow(2, -y));
	}

	void onKeyDown(int key) override
	{
		switch (key)
		{
		case GLFW_KEY_W: m_wDown = true; break;
		case GLFW_KEY_S: m_sDown = true; break;
		case GLFW_KEY_A: m_aDown = true; break;
		case GLFW_KEY_D: m_dDown = true; break;
		case GLFW_KEY_SPACE: m_spaceDown = true; break;
		case GLFW_KEY_LEFT_SHIFT: m_shiftDown = true; break;
		}
	}

	void onKeyUp(int key) override
	{
		switch (key)
		{
		case GLFW_KEY_W: m_wDown = false; break;
		case GLFW_KEY_S: m_sDown = false; break;
		case GLFW_KEY_A: m_aDown = false; break;
		case GLFW_KEY_D: m_dDown = false; break;
		case GLFW_KEY_SPACE: m_spaceDown = false; break;
		case GLFW_KEY_LEFT_SHIFT: m_shiftDown = false; break;
		}
	}


	void tick(float dt) override
	{
		dt *= s_speed;
		if (m_wDown)
			s_position += dt * s_direction;
		if (m_sDown)
			s_position -= dt * s_direction;
		if(m_aDown)
			s_position -= dt * glm::normalize(glm::cross(s_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
		if (m_dDown)
			s_position += dt * glm::normalize(glm::cross(s_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
		if(m_spaceDown)
			s_position += dt * glm::vec3(0.0f, 1.0f, 0.0f);
		if (m_shiftDown)
			s_position -= dt * glm::vec3(0.0f, 1.0f, 0.0f);

		calcProjection();
	}

	void onSizeChange(int width, int height) override
	{
		m_aspect = float(width) / float(height);
		calcProjection();
	}
private:
	void calcProjection()
	{
		m_projection = glm::perspective(s_fov * 3.1415926f / 180.0f, m_aspect, s_nearPlane, s_farPlane)
			* glm::lookAt(s_position, s_position + s_direction, glm::vec3(0.0f, 1.0f, 0.0f));
	}

private:
	float m_aspect = 1.0f;
	glm::mat4 m_projection;

	bool m_mouseDown = false;
	bool m_wDown = false;
	bool m_aDown = false;
	bool m_sDown = false;
	bool m_dDown = false;
	bool m_spaceDown = false;
	bool m_shiftDown = false;
};
