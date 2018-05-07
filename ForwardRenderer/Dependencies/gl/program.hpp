#pragma once
#include "shader.hpp"
#include <cassert>
#include <vector>

namespace gl
{
	class Program
	{
		explicit Program(bool)
		{}
	public:
		static Program empty()
		{
			return Program(false);
		}
		Program()
			:
		m_id(glCreateProgram())
		{}
		~Program()
		{
			glDeleteProgram(m_id);
		}
		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
		Program(Program&&) = default;
		Program& operator=(Program&&) = default;

		void bind() const
		{
			assert(m_id);
			glUseProgram(m_id);
		}
		static void unbind()
		{
			glUseProgram(0);
		}

		Program& attach(const Shader& shader)
		{
			assert(m_id);
			assert(shader.getId());
			glAttachShader(m_id, shader.getId());
			m_attachments.push_back(shader.getId());
			return *this;
		}

		Program& link()
		{
			glLinkProgram(m_id);

			// detach attachments
			for (const auto& s : m_attachments)
				glDetachShader(m_id, s);
			m_attachments.clear();

			// test link status
			GLint isLinked = 0;
			glGetProgramiv(m_id, GL_LINK_STATUS, &isLinked);
			if (isLinked == GL_FALSE)
				throw std::runtime_error(getInfoLog());
			
			return *this;
		}

		std::string getInfoLog() const
		{
			GLint length = 0;
			glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
			if (!length)
				return "";

			std::string errorLog;
			errorLog.reserve(length);
			glGetProgramInfoLog(m_id, length, &length, &errorLog[0]);
			return errorLog;
		}

		std::string getBinary() const
		{
			assert(m_id);
			GLint size = 0;
			glGetProgramiv(m_id, GL_PROGRAM_BINARY_LENGTH, &size);
			GLsizei bytesWritten = 0;
			GLenum binaryFormat = 0;
			std::string res;
			res.resize(size);
			glGetProgramBinary(m_id, size, &bytesWritten, &binaryFormat, res.data());

			return res;
		}

		static std::string convertLog(const std::string& log, const std::function<std::string(Shader::Type, GLint)>& convertFunction)
		{
			std::unordered_map<Shader::Type, std::regex> regexs;
			regexs[Shader::Type::FRAGMENT] = std::regex("Fragment info");
			regexs[Shader::Type::VERTEX] = std::regex("Vertex info");
			regexs[Shader::Type::GEOMETRY] = std::regex("Geometry info");
			regexs[Shader::Type::COMPUTE] = std::regex("Compute info");
			regexs[Shader::Type::TESS_CONTROL] = std::regex("Tess Control info");
			regexs[Shader::Type::TESS_EVALUATION] = std::regex("Tess Evaluation info");

			// search matches
			std::vector<std::pair<Shader::Type, std::smatch>> matches;
			matches.reserve(regexs.size());

			std::smatch m;
			for(const auto& rgx : regexs)
			{
				// assuming all matches can only happen once
				if(std::regex_search(log, m, rgx.second))
				{
					matches.push_back({ rgx.first, m });
				}
			}

			// sort matches
			std::sort(matches.begin(), matches.end(), []
			(const std::pair<Shader::Type, std::smatch>& left, const std::pair<Shader::Type, std::smatch>& right)
			{
				return left.second.prefix().str().length() < right.second.prefix().str().length();
			});

			if (matches.size() == 0) return log;
			// obtain corrected info logs

			std::string newLog = matches[0].second.prefix().str();
			for(auto i = 0; i < matches.size(); ++i)
			{
				std::string logPart = matches[i].second.suffix().str();
				// get the string
				if(i != matches.size() - 1)
				{
					// only validate the part between this match and the next match
					auto strlen = logPart.length() - matches[i + 1].second.suffix().length();
					logPart = logPart.substr(0, strlen);
				}

				// use the shader conversion function
				newLog += matches[i].second.str() + 
					Shader::convertLog(logPart, [&convertFunction, type = matches[i].first](const auto& id)
				{
					return convertFunction(type, id);
				});
			}

			return newLog;
		}
	private:
		unique<GLuint> m_id;
		std::vector<GLuint> m_attachments;
	};
}
