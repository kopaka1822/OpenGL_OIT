#include "EnvironmentRenderer.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

EnvironmentRenderer::EnvironmentRenderer()
{
	auto vert = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/Environment.vs");
	auto frag = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/Environment.fs");

	m_shader = std::make_unique<FullscreenQuadShader>(HotReloadShader::loadProgram({vert, frag}));
}

void EnvironmentRenderer::render(const RenderArgs& args)
{
	if (!args.environment) return;

	args.environment->bind();

	glDisable(GL_DEPTH_TEST);

	// set uniform parameters
	m_shader->bind();
	auto mat = calcTransform();
	auto plane = calcFarplane();

	glUniformMatrix4fv(0, 1, false, glm::value_ptr(mat));
	glUniform1f(1, plane);

	m_shader->draw();

	glEnable(GL_DEPTH_TEST);
}

void EnvironmentRenderer::onMouseMove(double x, double y, double dx, double dy)
{
	if (!m_mouseDown) return;

	m_pitch -= float(dx) * 0.01f * m_aperture;
	m_roll += float(dy) * 0.01f * m_aperture;
}

void EnvironmentRenderer::onButtonDown(int button)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		m_mouseDown = true;
}

void EnvironmentRenderer::onButtonUp(int button)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		m_mouseDown = false;
}

void EnvironmentRenderer::onScroll(double x, double y)
{
	auto step = y > 0.0f ? 1.0f / 1.1f : 1.1f;
	auto value = std::pow(step, std::abs(y));

	m_aperture = glm::clamp(m_aperture * float(value), 0.01f, 2.9f);
}

glm::mat4 EnvironmentRenderer::calcTransform() const
{
	return 
		glm::rotate(glm::mat4(), m_pitch, glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::rotate(glm::mat4(), m_roll, glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::scale(glm::mat4(), glm::vec3(-1.0f * float(Window::getWidth()) / float(Window::getHeight()), 1.0f, 1.0f));
}

float EnvironmentRenderer::calcFarplane() const
{
	return float(1.0f / std::tan(m_aperture / 2.0f));
}
