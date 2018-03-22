#include "HotReloadShader.h"
#include "../Dependencies/filewatcher/include/FileWatcher/FileWatcher.h"
#include <unordered_map>
#include <fstream>
#include <iostream>

struct WatchedDirectory
{
	WatchedDirectory() = default;
	explicit WatchedDirectory(FW::WatchID id)
		: id(id)
	{
	}

	FW::WatchID id = -1;
	std::vector<std::shared_ptr<HotReloadShader::WatchedShader>> shader;
};

// watched shaders with key = directory
static std::unordered_map<std::string, WatchedDirectory> s_watchedShader;
static std::vector<std::shared_ptr<HotReloadShader::WatchedProgram>> s_watchedPrograms;

class HotReloadShader::Listener : public FW::FileWatchListener
{
	
public:
	void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action) override
	{
		if (action != FW::Action::Modified && action != FW::Action::Add)
			return; // doesnt matter

		// find directory
		auto it = s_watchedShader.find(dir);
		if (it == s_watchedShader.end())
			return;

		// was an important file modified?
		auto& loaded = it->second.shader;

		const auto modifiedShader = std::find_if(loaded.begin(), loaded.end(), [&filename](const auto& shader)
		{
			return shader->getFilename() == filename;
		});

		if (modifiedShader == loaded.end())
			return; // no file modified

		auto& watchedShader = **modifiedShader;
		const auto fullFilename = dir + "/" + filename;
		// try a hot reload
		std::cerr << "attempting hot reload for " << fullFilename << "\n";

		try
		{
			HotReloadShader::loadShader(watchedShader);
			// TODO check if binary has changed?
			std::cerr << "shader reload was succesfull\n";
		}
		catch(const std::exception& e)
		{
			std::cerr << "hot reload failed\n";
			std::cerr << e.what() << "\n";
		}
		
		for(auto& program : s_watchedPrograms)
		{
			if(program->hasShader(watchedShader)) try
			{
				// relink
				HotReloadShader::loadProgram(*program);
			}
			catch(const std::exception& e)
			{
				std::cerr << "program relink failed for" << program->getDescription() << "\n";
				std::cerr << e.what() << "\n";
			}
		}
	}
};

static HotReloadShader::Listener s_listener;
static FW::FileWatcher s_fileWatcher;

void HotReloadShader::update()
{
	s_fileWatcher.update();
}

std::shared_ptr<HotReloadShader::WatchedShader> HotReloadShader::loadShader(gl::Shader::Type type,
	const std::string& directory, const std::string& filename)
{
	// is directory being watched?
	if(s_watchedShader.find(directory) == s_watchedShader.end())
	{
		// add watch for directory
		const auto watchId = s_fileWatcher.addWatch(directory, &s_listener, false);
		// add entry
		s_watchedShader[directory] = WatchedDirectory(watchId);
	}
	
	// is the shader already loaded?
	auto& loaded = s_watchedShader[directory].shader;
	auto it = std::find_if(loaded.begin(), loaded.end(), [&filename](const auto& shader)
	{
		return shader->getFilename() == filename;
	});
	
	if (it != loaded.end())
		return *it;

	// create shared ptr
	auto newShader = std::shared_ptr<WatchedShader>(new WatchedShader(type, directory, filename));

	// load shader source
	loadShader(*newShader);

	// add to file watcher list
	loaded.push_back(newShader);

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
