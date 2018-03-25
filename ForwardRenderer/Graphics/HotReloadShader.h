#pragma once
#include <memory>
#include "../Dependencies/gl/shader.hpp"
#include "../Dependencies/gl/program.hpp"
#include <numeric>
#include <filesystem>
#include <map>
namespace fs = std::experimental::filesystem;

struct HotReloadShader
{
	class Listener;
	friend Listener;

	struct WatchedPath
	{
		WatchedPath() = default;
		WatchedPath(fs::path path, time_t modified) 
		: path(std::move(path)), lastModified(modified) 
		{}

		fs::path path;
		time_t lastModified = 0;
	};

	class WatchedShader
	{
		friend HotReloadShader;

		WatchedShader(gl::Shader::Type shaderType, fs::path path)
			: m_type(shaderType),
			  m_path(std::move(path))
		{
		}
		bool hasDependency(const fs::path& path) const
		{
			return m_path == path ||
			std::any_of(m_usedFiles.begin(), m_usedFiles.end(), [&path](const auto& other)
			{
				return other->path == path;
			});
		}
		std::string getDescription() const
		{
			return m_path.string();
		}
		std::string getFilename() const
		{
			return m_path.filename().string();
		}
	public:
		using PathMap = std::map<fs::path, size_t>;

		const gl::Shader& getShader() const { return m_shader; }
	private:
		gl::Shader m_shader;
		gl::Shader::Type m_type;
		fs::path m_path;
		std::vector<std::shared_ptr<WatchedPath>> m_usedFiles;
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
				return ws.get() == &shader;
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
	 * \param filename shader directory + filename
	 * \return 
	 */
	static std::shared_ptr<WatchedShader> loadShader(gl::Shader::Type type, const fs::path& filename);
	static std::shared_ptr<WatchedProgram> loadProgram(std::initializer_list<std::shared_ptr<WatchedShader>> shader);


private:
	static void loadShader(WatchedShader& dest);
	static void loadProgram(WatchedProgram& program);
};
