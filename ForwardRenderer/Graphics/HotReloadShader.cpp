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
#include "../ScriptEngine/ScriptEngine.h"
namespace fs = std::experimental::filesystem;

#ifndef WIN32
#include <unistd.h>
#define stat _stat
#endif

time_t getLastModified(const std::string& filename)
{
	struct stat result{};
	if (stat(filename.c_str(), &result) != 0)
		return 0;

	return result.st_mtime;
}

// watched shaders with key = directory
static std::vector<std::shared_ptr<HotReloadShader::WatchedShader>> s_watchedShader;
static std::vector<std::shared_ptr<HotReloadShader::WatchedPath>> s_usedFiles;
static std::vector<std::shared_ptr<HotReloadShader::WatchedProgram>> s_watchedPrograms;

static std::chrono::steady_clock::time_point s_lastUpdate = std::chrono::steady_clock::now();

void HotReloadShader::initScripts()
{
	ScriptEngine::addFunction("getActiveShader", [](const auto&)
	{
		update(true);
		return std::accumulate(s_watchedShader.begin(), s_watchedShader.end(), std::string(), [](const auto& prev, const auto& cur)
		{
			return prev + "   " + cur->getFilename() + "\n";
		});
	});
	ScriptEngine::addFunction("saveShaderBinary", [](const std::vector<Token>& args)
	{
		if (args.size() < 2)
			throw std::runtime_error("expected shader name and destination filename as arguments");

		// force update current shaders
		update(true);

		// find the shader
		auto shader = std::find_if(s_watchedShader.begin(), s_watchedShader.end(), [name = args.at(0).getString()](const auto& shader)
		{
			return shader->getFilename() == name;
		});
		if (shader == s_watchedShader.end())
			throw std::runtime_error("could not find shader with filename: " + args.at(0).getString());

		// find the program in which this shader is used
		auto program = std::find_if(s_watchedPrograms.begin(), s_watchedPrograms.end(), [&shader](const auto& program)
		{
			return program->hasShader(**shader);
		});
		if (program == s_watchedPrograms.end())
			throw std::runtime_error("could not find any associated programs for the shader");

		// obtain binary
		auto binary = (*program)->getProgram().getBinary();

		// save binary in file
		std::fstream file;
		file.open(args.at(1).getString(), std::ios::out | std::ios::binary | std::ios::trunc);
		if (!file.is_open())
			throw std::runtime_error("could not open destination file");

		file << binary;

		return "";
	});
}


void HotReloadShader::update(bool force)
{
	const auto now = std::chrono::steady_clock::now();
	if (!force && std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastUpdate).count() < 500)
		return; // dont spam updates

	s_lastUpdate = now;

	const auto removeSingleSharedPtr = [](auto& vector)
	{
		const auto it = std::remove_if(vector.begin(), vector.end(), [](const auto& thingy)
		{
			return thingy.use_count() <= 1;
		});
		if (it != vector.end())
			vector.erase(it, vector.end());
	};

	removeSingleSharedPtr(s_watchedPrograms);
	removeSingleSharedPtr(s_watchedShader);
	removeSingleSharedPtr(s_usedFiles);

	for (const auto& path : s_usedFiles)
	{
		const auto lastModified = getLastModified(path->path.string());
		if (lastModified > path->lastModified)
		{
			// file was modified
			std::cerr << "file " << path->path << " was modified\n";
			path->lastModified = lastModified;

			// TODO maybe just mark shader for recompilation for the case that multiple files have changed

			// do a hot reload for all affected shader
			for (const auto& shader : s_watchedShader)
			{
				if (!shader->hasDependency(path->path))
					continue;

				// file was modified
				// try a hot reload
				std::cerr << "attempting hot reload for " << shader->getDescription() << '\n';

				try
				{
					loadShader(*shader);
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
					if (program->hasShader(*shader))
						try
						{
							// relink
							loadProgram(*program);
						}
						catch (const std::exception& e)
						{
							std::cerr << "program relink failed for " << program->getDescription() << '\n';
							std::cerr << e.what() << '\n';
						}
				}
				std::cerr << "finished program reload\n";
			}
		}
	}
}

std::shared_ptr<HotReloadShader::WatchedShader> HotReloadShader::loadShader(gl::Shader::Type type,
                                                                            const fs::path& filename, size_t glVersion,
                                                                            std::string preamble)
{
	const auto it = std::find_if(s_watchedShader.begin(), s_watchedShader.end(), [=](const auto& shader)
	{
		return shader->matches(filename, glVersion, preamble);
	});
	if (it != s_watchedShader.end())
		return *it;

	auto newShader = std::shared_ptr<WatchedShader>(new WatchedShader(type, filename, glVersion, std::move(preamble)));

	// load shader source
	loadShader(*newShader);

	// add entry
	s_watchedShader.push_back(newShader);

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
	for (auto& c : string)
	{
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

	const auto fileNumber = usedFiles.size() + 1;
	usedFiles[filename] = fileNumber;

	std::ifstream file;
	file.open(filename);

	std::string output = "#line 1 " + std::to_string(fileNumber) + "\n";

	std::string line;

	if (!file.is_open())
		throw std::runtime_error("could not open file " + filename.string());

	size_t lineNumber = 0;
	while (file.good())
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

	WatchedShader::PathMap usedFiles;
	usedFiles["<preamble>"] = 0;

	auto output =
		"#version " + std::to_string(dest.getVersion()) + "\n" +
		"#pragma optionNV(strict on)\n"
		"#line 1\n" +
		dest.getPreamble() + "\n" +		
		loadShaderSource(path, usedFiles);



	// make the file map function
	std::function<std::string(GLint)> fileMatcher = [usedFiles](GLint num) -> std::string
	{
		for (const auto& file : usedFiles)
			if (file.second == num)
				return file.first.string();
		
		return "?";
	};


	// compile shader
	const char* source = output.c_str();
	try
	{
		auto string = path.string();
		shader.compile(source, string.c_str());
		auto log = shader.getInfoLog();
		if (log.length())
			std::cerr << gl::Shader::convertLog(log, fileMatcher) << '\n';
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(gl::Shader::convertLog(e.what(), fileMatcher));
	}

	// set new shader
	dest.m_shader = std::move(shader);

	// update dependencies
	dest.m_fileMatcher = fileMatcher;
	dest.m_usedFiles.clear();
	for (const auto& file : usedFiles)
	{
		// path already in used paths?
		const auto it = std::find_if(s_usedFiles.begin(), s_usedFiles.end(), [&file](const auto& pathPtr)
		{
			return pathPtr->path == file.first;
		});
		if (it != s_usedFiles.end())
		{
			dest.m_usedFiles.push_back(*it);

		}
		else
		{
			// update last modified
			auto lastModified = getLastModified(file.first.string());

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

	try
	{
		p.link();
		auto log = p.getInfoLog();
		if (log.length())
			std::cerr << gl::Program::convertLog(log, program.getConvertFunction()) << '\n';
	}
	catch(const std::exception& e)
	{
		throw std::runtime_error(gl::Program::convertLog(e.what(), program.getConvertFunction()));
	}

	// assign new program
	program.m_program = std::move(p);
}
