#pragma once
#include <memory>
#include "../Dependencies/gl/shader.hpp"
#include "../Dependencies/gl/program.hpp"
#include <numeric>

struct HotReloadShader
{
	class Listener;
	friend Listener;

	class WatchedShader
	{
		friend HotReloadShader;
		WatchedShader(gl::Shader::Type shaderType, std::string directory, std::string filename)
			: m_type(shaderType),
			  m_directory(std::move(directory)),
			  m_filename(std::move(filename)),
			  m_lastModified(0)
		{
		}
		bool hasShader(const WatchedShader& shader) const
		{
			// TODO add includes
			return m_filename == shader.m_filename && m_directory == shader.m_directory;
		}
		std::string getDescription() const
		{
			return m_directory + "/" + m_filename;
		}
		const std::string& getFilename() const
		{
			return m_filename;
		}
	public:
		const gl::Shader& getShader() const { return m_shader; }
	private:
		gl::Shader m_shader;
		gl::Shader::Type m_type;
		std::string m_directory;
		std::string m_filename;
		time_t m_lastModified;
	};

	class WatchedProgram
	{
		friend HotReloadShader;
		WatchedProgram(std::initializer_list<std::shared_ptr<WatchedShader>> usedShader)
			:
		m_usedShader(usedShader)
		{
		}
		bool hasShader(const WatchedShader& shader) const
		{
			return std::any_of(m_usedShader.begin(), m_usedShader.end(), [&shader](const auto& ws)
			{
				return ws->hasShader(shader);
			});
		}
		std::string getDescription() const
		{
			return std::accumulate(m_usedShader.begin(), m_usedShader.end(), std::string("program("), [](std::string res, const auto& shader)
			{
				return res + shader->getDescription() + " ";
			}) + ")";
		}
	public:

		const gl::Program& getProgram() const { return m_program; }
	private:
		gl::Program m_program;
		std::vector<std::shared_ptr<WatchedShader>> m_usedShader;
	};

	static void update();

	/**
	 * \brief 
	 * \param type shader type
	 * \param directory directory (without / at the end)
	 * \param filename shader file in directory
	 * \return 
	 */
	static std::shared_ptr<WatchedShader> loadShader(gl::Shader::Type type, const std::string& directory, const std::string& filename);
	static std::shared_ptr<WatchedProgram> loadProgram(std::initializer_list<std::shared_ptr<WatchedShader>> shader);


private:
	static void loadShader(WatchedShader& dest);
	static void loadProgram(WatchedProgram& program);
};
