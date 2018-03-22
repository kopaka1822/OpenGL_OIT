#include "HotReloadShader.h"
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#define stat _stat
#endif

time_t getLastModified(const std::string& filename)
{
	struct stat result {};
	if (stat(filename.c_str(), &result) != 0)
		return 0;

	return result.st_mtime;
}

// watched shaders with key = directory
static std::unordered_map<std::string, std::shared_ptr<HotReloadShader::WatchedShader>> s_watchedShader;
static std::vector<std::shared_ptr<HotReloadShader::WatchedProgram>> s_watchedPrograms; 
static std::chrono::steady_clock::time_point s_lastUpdate = std::chrono::steady_clock::now();

void HotReloadShader::update()
{
	const auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastUpdate).count() < 500)
		return; // dont spam updates
	
	s_lastUpdate = now;

	for(const auto& shader : s_watchedShader)
	{
		if(getLastModified(shader.first) > shader.second->m_lastModified)
		{
			// file was modified
			// try a hot reload
			std::cerr << "attempting hot reload for " << shader.first << "\n";

			try
			{
				loadShader(*shader.second);
				// TODO check if binary has changed?
				std::cerr << "shader reload was succesfull\n";
			}
			catch (const std::exception& e)
			{
				std::cerr << "hot reload failed\n";
				std::cerr << e.what() << "\n";
				continue;
			}

			for (auto& program : s_watchedPrograms)
			{
				if (program->hasShader(*shader.second)) try
				{
					// relink
					loadProgram(*program);
				}
				catch (const std::exception& e)
				{
					std::cerr << "program relink failed for " << program->getDescription() << "\n";
					std::cerr << e.what() << "\n";
				}
			}
			std::cerr << "finished program reload\n";
		}
	}
}

std::shared_ptr<HotReloadShader::WatchedShader> HotReloadShader::loadShader(gl::Shader::Type type,
	const std::string& directory, const std::string& filename)
{
	const auto fullFilename = directory + "/" + filename;
	// is directory being watched?

	const auto it = s_watchedShader.find(fullFilename);
	if (it != s_watchedShader.end())
		return it->second;
	
	auto newShader = std::shared_ptr<WatchedShader>(new WatchedShader(type, directory, filename));

	// load shader source
	loadShader(*newShader);

	// add entry
	s_watchedShader[fullFilename] = newShader;

	return newShader;
}

std::shared_ptr<HotReloadShader::WatchedProgram> HotReloadShader::loadProgram(
	std::initializer_list<std::shared_ptr<WatchedShader>> shader)
{
	// TODO is there a program with the same properties

	auto newProgram = std::shared_ptr<WatchedProgram>(new WatchedProgram(shader));

	loadProgram(*newProgram);

	s_watchedPrograms.push_back(newProgram);

	return newProgram;
}

void HotReloadShader::loadShader(WatchedShader& dest)
{
	// create new shader object
	gl::Shader shader(dest.m_type);

	const auto filename = dest.m_directory + "/" + dest.m_filename;

	// update last modified
	dest.m_lastModified = getLastModified(filename);

	// load file
	std::ifstream file;
	file.open(filename);

	std::string output;
	std::string line;
	if (!file.is_open())
		throw std::runtime_error("could not open file " + filename);


	while (file.good())
	{
		std::getline(file, line);
		// TODO add inlcude
		output.append(line + "\n");
	}

	// compile shader
	const char* source = output.c_str();
	shader.compile(1, &source, filename.c_str());

	// set new shader
	dest.m_shader = std::move(shader);
}

void HotReloadShader::loadProgram(WatchedProgram& program)
{
	gl::Program p;

	// attach all shader
	for (const auto& s : program.m_usedShader)
		p.attach(s->getShader());

	p.link();

	// assign new program
	program.m_program = std::move(p);
}
