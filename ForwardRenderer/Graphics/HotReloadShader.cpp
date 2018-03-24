#include "HotReloadShader.h"
#include <unordered_map>
#include <map>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

#include <filesystem>
#include <regex>
namespace fs = std::experimental::filesystem;

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
static std::vector<std::shared_ptr<HotReloadShader::WatchedPath>> s_usedFiles;
static std::vector<std::shared_ptr<HotReloadShader::WatchedProgram>> s_watchedPrograms; 
static std::chrono::steady_clock::time_point s_lastUpdate = std::chrono::steady_clock::now();

void HotReloadShader::update()
{
	const auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastUpdate).count() < 500)
		return; // dont spam updates
	
	s_lastUpdate = now;

	// remove unused programs
	{
		const auto it = std::remove_if(s_watchedPrograms.begin(), s_watchedPrograms.end(), [](const auto& program)
		{
			return program.use_count() <= 1;
		});
		if (it != s_watchedPrograms.end())
			s_watchedPrograms.erase(it, s_watchedPrograms.end());
	}

	// remove unused shader
	{
		for(auto it = s_watchedShader.begin(); it != s_watchedShader.end();)
		{
			if(it->second.use_count() <= 1)
			{
				it = s_watchedShader.erase(it);
			}
			else ++it;
		}
	}

	// remove unused files
	{
		const auto it = std::remove_if(s_usedFiles.begin(), s_usedFiles.end(), [](const auto& path)
		{
			return path.use_count() <= 1;
		});
		if (it != s_usedFiles.end())
			s_usedFiles.erase(it, s_usedFiles.end());
	}

	for(const auto& path : s_usedFiles)
	{
		const auto lastModified = getLastModified(path->path.string());
		if (lastModified > path->lastModified)
		{
			// file was modified
			std::cerr << "file " << path->path << " was modified\n";
			path->lastModified = lastModified;

			// TODO maybe just mark shader for recompilation for the case that multiple files have changed

			// do a hot reload for all affected shader
			for(const auto& shader : s_watchedShader)
			{
				if(!shader.second->hasDependency(path->path))
					continue;

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
}

std::shared_ptr<HotReloadShader::WatchedShader> HotReloadShader::loadShader(gl::Shader::Type type,
	const std::string& directory, const std::string& filename)
{
	const auto fullFilename = directory + "/" + filename;
	// is directory being watched?

	const auto it = s_watchedShader.find(fullFilename);
	if (it != s_watchedShader.end())
		return it->second;
	
	auto newShader = std::shared_ptr<WatchedShader>(new WatchedShader(type, fullFilename));

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

/**
 * \brief tries to remove as many ../ as possible
 * \param path path that should be cleansed
 * \return path without ../ if possible (only if they could be removed)
 */
fs::path cleanPath(const fs::path& path)
{
	auto string = path.wstring();

	// replace \\ with /
	for (auto& c : string) {
		if (c == wchar_t('\\'))
			c = wchar_t('/');
	}
	// remove all ../
	auto pos = string.find(L"../");
	while (pos != std::wstring::npos)
	{
		if (pos != 0)
		{
			// is another ../ before this pos?
			bool skipThis = false;
			if (pos > 2)
			{
				skipThis = string.at(pos - 1) == wchar_t('/')
					&& string.at(pos - 2) == wchar_t('.') &&
					string.at(pos - 3) == wchar_t('.');
			}

			if (!skipThis)
			{
				// remove the previous path
				// find previous /
				auto prevSlash = string.find_last_of(wchar_t('/'), pos - 2); // dont take the first / from /../
				if (prevSlash == std::wstring::npos)
					prevSlash = 0; // no previous directory

				if (pos > prevSlash)
				{
					// remove this part
					if (prevSlash == 0)
						string = string.substr(pos + 3);
					else
						string = string.substr(0, prevSlash + 1) + string.substr(pos + 3);

					pos = prevSlash;
				}
			}
		}
		pos = string.find(L"../", pos + 1);
	}

	return fs::path(string);
}

std::string loadShaderSource(fs::path filename, HotReloadShader::WatchedShader::PathMap& usedFiles)
{
	filename = cleanPath(filename);

	// is the file already included?
	if (usedFiles.find(filename) != usedFiles.end())
		return "\n"; // already included
	// TODO warning for circular dependencies?

	const auto fileNumber = usedFiles.size();
	usedFiles[filename] = fileNumber;

	std::ifstream file;
	file.open(filename);

	std::string output;
	
	if(fileNumber != 0) // dont put this in the first file because: the first statement must be the version number
		output = "#line 1 " + std::to_string(fileNumber) + "\n";

	std::string line;

	if (!file.is_open())
		throw std::runtime_error("could not open file " + filename.string());

	size_t lineNumber = 0;
	while(file.good())
	{
		std::getline(file, line);
		++lineNumber;

		// check for #include 
		// check line for #include

		const auto includeStart = line.find("#include");
		auto commentStart = std::string::npos;
		if (includeStart != std::string::npos) // commented out?
			commentStart = line.find("//");

		if (includeStart != std::string::npos && includeStart < commentStart)
		{
			// we have an include
			const auto parStart = line.find('\"', includeStart + strlen("#include"));
			if (parStart == std::string::npos)
				throw std::runtime_error("missing 1. \" for include");

			const auto parEnd = line.find('\"', parStart + 1);
			if (parEnd == std::string::npos)
				throw std::runtime_error("missing 2. \" for include");

			const auto includeFile = line.substr(parStart + 1, parEnd - parStart - 1);
			// put path together
			const auto includePath = filename.parent_path().append(includeFile);

			// parse this file
			output += loadShaderSource(includePath, usedFiles);
			output += "\n#line " + std::to_string(lineNumber + 1) + " " + std::to_string(fileNumber) + '\n';
		}
		else
		{
			output.append(line + '\n');
		}
	}

	return output;
}

void HotReloadShader::loadShader(WatchedShader& dest)
{
	// create new shader object
	gl::Shader shader(dest.m_type);

	const auto& path = dest.m_path;

	// update last modified
	auto lastModified = getLastModified(path.string());

	WatchedShader::PathMap usedFiles;

	auto output = loadShaderSource(path, usedFiles);

	// compile shader
	const char* source = output.c_str();
	try
	{
		auto string = path.string();
		shader.compile(1, &source, string.c_str());
	}
	catch(const std::exception& e)
	{
		// convert error information with used files table
		// errors are like \n5(20): => error in file 5 line 20
		//const std::regex expr("\n[0-9][0-9]*\\([0-9][0-9]*\\):");
		const std::regex expr("[0-9][0-9]*\\([0-9][0-9]*\\)");
		std::smatch m;

		std::string error;
		std::string remaining = e.what();

		while(std::regex_search(remaining, m, expr))
		{
			error += m.prefix();

			// append the correct filename
			// extract number
			const auto parOpen = m.str().find('(');
			const auto fileNumber = m.str().substr(0, parOpen);

			const size_t num = std::stoi(fileNumber);
			for(const auto& file : usedFiles)
			{
				if(file.second == num)
				{
					// thats the match
					error += file.first.string();
					error += m.str().substr(parOpen);
					break;
				}
			}

			remaining = m.suffix().str();
		}

		error += remaining;

		throw std::runtime_error(error);
	}

	// set new shader
	dest.m_shader = std::move(shader);

	// update dependencies
	dest.m_usedFiles.clear();
	for(const auto& file : usedFiles)
	{
		// path already in used paths?
		const auto it = std::find_if(s_usedFiles.begin(), s_usedFiles.end(), [&file](const auto& pathPtr)
		{
			return pathPtr->path == file.first;
		});
		if(it != s_usedFiles.end())
		{
			dest.m_usedFiles.push_back(*it);
		}
		else
		{
			const auto ptr = std::make_shared<WatchedPath>(file.first, lastModified);
			dest.m_usedFiles.push_back(ptr);
			s_usedFiles.push_back(ptr);
		}
	}
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
