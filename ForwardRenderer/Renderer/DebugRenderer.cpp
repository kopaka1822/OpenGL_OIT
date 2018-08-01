#include "DebugRenderer.h"
#include <glad/glad.h>
#include "../Graphics/HotReloadShader.h"
#include "../Implementations/SimpleShader.h"
#include "../ScriptEngine/ScriptEngine.h"

enum class Type
{
	Normal,
	Texcoord,
	Mesh,
	SolidMesh,
	Depth
};
static Type s_type = Type::Normal;

DebugRenderer::DebugRenderer()
{
	auto vertex = HotReloadShader::loadShader(gl::Shader::Type::VERTEX, "Shader/DefaultShader.vs");
	auto geometry = HotReloadShader::loadShader(gl::Shader::Type::GEOMETRY, "Shader/DefaultShader.gs");
	auto normal = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/NormalColor.fs", 430, "#define DEBUG_NORMAL");
	auto texcoord = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/NormalColor.fs");
	auto mesh = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/NormalColor.fs", 430, "#define DEBUG_MESH");
	auto depth = HotReloadShader::loadShader(gl::Shader::Type::FRAGMENT, "Shader/NormalColor.fs", 430, "#define DEBUG_DEPTH");

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

	m_depthShader = std::make_unique<SimpleShader>(
		HotReloadShader::loadProgram({ vertex, geometry, depth }));
	if (s_type == Type::Depth)
		m_activeShader = m_depthShader.get();
}

DebugRenderer::~DebugRenderer()
{
	ScriptEngine::removeProperty("debugType");
}

void DebugRenderer::render(const RenderArgs& args)
{
	if(!args.camera || !args.transforms || !args.model)
		return;

	// clear color
	if (s_type == Type::Mesh || s_type == Type::SolidMesh || s_type == Type::Depth)
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	// clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	
	// depth pre pass?
	if(s_type == Type::SolidMesh)
	{
		// fill z buffer
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		args.model->prepareDrawing(*m_activeShader);
		for (const auto& s : args.model->getShapes())
			s->draw(m_activeShader);

		glDepthFunc(GL_LEQUAL);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	if (s_type == Type::Mesh || s_type == Type::SolidMesh)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	args.model->prepareDrawing(*m_activeShader);
	for (const auto& s : args.model->getShapes())
		s->draw(m_activeShader);

	if (s_type == Type::Mesh || s_type == Type::SolidMesh)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthFunc(GL_LESS);
	}
}

void DebugRenderer::init()
{
	ScriptEngine::addKeyword("normal");
	ScriptEngine::addKeyword("texcoord");
	ScriptEngine::addKeyword("mesh");
	ScriptEngine::addKeyword("solid_mesh");
	ScriptEngine::addKeyword("depth");

	ScriptEngine::addProperty("debugType", [this]()
	{
		switch (s_type)
		{
		case Type::Normal: return "normal";
		case Type::Texcoord: return "texcoord";
		case Type::Mesh: return "mesh";
		case Type::SolidMesh: return "solid_mesh";
		case Type::Depth: return "depth";
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
		else if (args.at(0).getString() == "solid_mesh")
		{
			s_type = Type::SolidMesh;
			m_activeShader = m_meshShader.get();
		}
		else if (args.at(0).getString() == "depth")
		{
			s_type = Type::Depth;
			m_activeShader = m_depthShader.get();
		}
		else
		{
			throw std::runtime_error("uknown debug type. try normal, texcoord, mesh, solid_mesh, depth");
		}
	});
}
