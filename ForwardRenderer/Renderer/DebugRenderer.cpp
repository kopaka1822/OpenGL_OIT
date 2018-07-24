#include "DebugRenderer.h"
#include <glad/glad.h>
#include "../Graphics/HotReloadShader.h"
#include "../Implementations/SimpleShader.h"
#include "../ScriptEngine/ScriptEngine.h"

enum class Type
{
	Normal,
	Texcoord,
	Mesh
};
static Type s_type = Type::Normal;

DebugRenderer::DebugRenderer()
{
	auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
	auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
	auto normal = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/NormalColor.fs", 430, "#define DEBUG_NORMAL");
	auto texcoord = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/NormalColor.fs");
	auto mesh = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/NormalColor.fs", 430, "#define DEBUG_MESH");

	m_normalShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({ vertex, geometry, normal }));
	if (s_type == Type::Normal)
		m_activeShader = m_normalShader.get();

	m_texcoordShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({ vertex, geometry, texcoord }));
	if (s_type == Type::Texcoord)
		m_activeShader = m_texcoordShader.get();

	m_meshShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({ vertex, geometry, mesh }));
	if (s_type == Type::Mesh)
		m_activeShader = m_meshShader.get();
}

DebugRenderer::~DebugRenderer()
{
	ScriptEngine::removeProperty("debugType");
}

void DebugRenderer::render(const RenderArgs& args)
{
	if(!args.camera || !args.transforms || !args.model)
		return;

	glEnable(GL_DEPTH_TEST);
	if(s_type == Type::Mesh)
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	args.model->prepareDrawing(*m_activeShader);
	for (const auto& s : args.model->getShapes())
		s->draw(m_activeShader);

	if (s_type == Type::Mesh)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void DebugRenderer::init()
{
	ScriptEngine::addProperty("debugType", [this]()
	{
		switch (s_type)
		{
		case Type::Normal: return "normal";
		case Type::Texcoord: return "texcoord";
		case Type::Mesh: return "mesh";
		}
		return "";
	}, [this](const std::vector<Token>& args) 
	{
		if(args.at(0).getString() == "normal")
		{
			s_type = Type::Normal;
			m_activeShader = m_normalShader.get();
		}
		else if(args.at(0).getString() == "texcoord")
		{
			s_type = Type::Texcoord;
			m_activeShader = m_texcoordShader.get();
		}
		else if (args.at(0).getString() == "mesh")
		{
			s_type = Type::Mesh;
			m_activeShader = m_meshShader.get();
		}
		else
		{
			throw std::runtime_error("uknown debug type. try normal, texcoord or mesh");
		}
	});
}
