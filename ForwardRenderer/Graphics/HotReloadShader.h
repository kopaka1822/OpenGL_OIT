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

		WatchedShader(gl::Shader::Type shaderType, fs::path path, size_t glVersion, std::string preamble)
			: m_type(shaderType),
			  m_path(std::move(path)),
			  m_glVersion(glVersion),
			  m_preamble(std::move(preamble))
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

		/**
		 * \brief verifies if this shader has the required properties
		 * \param path shader path
		 * \param glVersion shader version
		 * \param preamble shader preamble
		 * \return true if arguments match
		 */
		bool matches(const fs::path& path, size_t glVersion, const std::string& preamble) const
		{
			return m_path == path && m_glVersion == glVersion && m_preamble == preamble;
		}

		/// \return glsl shader version
		size_t getVersion() const { return m_glVersion; }
		const std::string& getPreamble() const { return m_preamble; }
	public:
		using PathMap = std::map<fs::path, size_t>;

		const gl::Shader& getShader() const { return m_shader; }
		gl::Shader::Type getType() const
		{
			return m_type;
		}
		const std::function<std::string(GLint)>& getConverter() const
		{
			return m_fileMatcher;
		}
	private:
		gl::Shader m_shader;
		gl::Shader::Type m_type;
		fs::path m_path;
		std::vector<std::shared_ptr<WatchedPath>> m_usedFiles;
		std::function<std::string(GLint)> m_fileMatcher;
		// glsl version
		size_t m_glVersion;
		// code block that will be inserted before the first file
		std::string m_preamble;
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
		std::function<std::string(gl::Shader::Type, GLint)> getConvertFunction()
		{
			return [this](gl::Shader::Type type, GLint id)
			{
				for(const auto& shader : m_usedShader)
				{
					if (shader->getType() == type)
						return shader->getConverter()(id);
				}
			};
		}
	private:
		gl::Program m_program;
		std::vector<std::shared_ptr<WatchedShader>> m_usedShader;
	};

	static void update(bool force = false);

	/**
	 * \brief 
	 * \param type shader type
	 * \param filename shader directory + filename
	 * \param glVersion the glsl version for the shader (default is 430)
	 * \param preamble an additional glsl code block that is inserted between the version number and the first file (can be used for defines)
	 * \return 
	 */
	static std::shared_ptr<WatchedShader> loadShader(gl::Shader::Type type, const fs::path& filename, size_t glVersion = 430, std::string preamble = "");
	static std::shared_ptr<WatchedProgram> loadProgram(std::initializer_list<std::shared_ptr<WatchedShader>> shader);

	static void initScripts();
private:
	static void loadShader(WatchedShader& dest);
	static void loadProgram(WatchedProgram& program);
};
