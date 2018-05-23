#include "IRenderer.h"
#include "../ScriptEngine/ScriptEngine.h"

glm::vec4 IRenderer::s_clearColor = glm::vec4(0.4666f, 0.709f, 0.87f, 0.99f);

void IRenderer::setClearColor()
{
	glClearColor(s_clearColor.r, s_clearColor.g, s_clearColor.b, s_clearColor.a);
}

void IRenderer::initScripts()
{
	ScriptEngine::addProperty("clearColor", []()
	{
		return std::to_string(s_clearColor.r) + ", " + std::to_string(s_clearColor.g) + ", " + std::to_string(s_clearColor.b) + ", " + std::to_string(s_clearColor.a);
	}, [](const std::vector<Token>& args)
	{
		if (args.size() == 2) throw std::runtime_error("expected 1, 3 or 4 arguments");

		s_clearColor = glm::vec4(args.at(0).getFloat());
		s_clearColor.a = 1.0f;

		if(args.size() >= 3)
		{
			s_clearColor.g = args.at(1).getFloat();
			s_clearColor.b = args.at(2).getFloat();
		}
		if(args.size() >= 4)
		{
			s_clearColor.a = args.at(3).getFloat();
		}
	});
}
