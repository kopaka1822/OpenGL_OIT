#include "ObjModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "Dependencies/tiny_obj_loader.h"
#include <iostream>
#include <chrono>
#include "Graphics/ElementBuffer.h"
#include "ObjShape.h"
#include "SimpleMaterial.h"
#include "SimpleMaterial.h"

// attempts to retrieve the file directory
static std::string GetDirectory(const std::string &filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\")) + "\\";
	return "";
}

ObjModel::ObjModel(const std::string& filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	const auto directory = GetDirectory(filename);

	auto time_load_start = std::chrono::high_resolution_clock::now();

	std::string err;
	bool res = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), directory.c_str(), true);
	if (!res)
		throw std::runtime_error(err.c_str());
	if (err.length())
		std::cerr << err << std::endl;

	std::cerr << "loading took " << std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::high_resolution_clock::now() - time_load_start
		).count() << " ms" << std::endl;

	printf("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
	printf("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
	printf("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
	printf("# of materials = %d\n", (int)materials.size());
	printf("# of shapes    = %d\n", (int)shapes.size());

	// Append `default` material
	materials.push_back(tinyobj::material_t());

	// make attribute buffer
	if (!attrib.vertices.size())
		throw std::runtime_error("no vertices found");

	m_vao = std::make_unique<VertexArrayObject>();
	m_vao->addArray(0, 0, 1, GL_INT, 0);
	m_vao->addArray(1, 0, 1, GL_INT, sizeof(int));
	m_vao->addArray(2, 0, 1, GL_INT, sizeof(int) * 2);

	m_vertices.reset(
		new ShaderStorageBuffer(attrib.vertices.size() * sizeof(attrib.vertices[0]), attrib.vertices.data(), 0));

	if (attrib.normals.size())
	{
		m_normals.reset(
			new ShaderStorageBuffer(attrib.normals.size() * sizeof(attrib.normals[0]), attrib.normals.data(), 0));

	}
	if (attrib.texcoords.size())
	{
		m_texcoords.reset(
			new ShaderStorageBuffer(attrib.texcoords.size() * sizeof(attrib.texcoords[0]), attrib.texcoords.data(), 0));
		
	}
	
	std::cerr << "INF: creating material" << std::endl;
	// load material
	m_material.reserve(materials.size());
	for(const auto& m : materials)
	{
		auto mat = std::make_unique<SimpleMaterial>();

		mat->addAttribute("diffuse", glm::vec4(m.diffuse[0], m.diffuse[1], m.diffuse[2], 1.0f));
		mat->addAttribute("ambient", glm::vec4(m.ambient[0], m.ambient[1], m.ambient[2], 1.0f));
		mat->addAttribute("specular", glm::vec4(m.specular[0], m.specular[1], m.specular[2], m.shininess));
		mat->addAttribute("dissolve", glm::vec4(m.dissolve));

		// add available attributes
		if (m.diffuse_texname.length())
			tryAddingTexture(*mat, "diffuse", directory + m.diffuse_texname);

		if (m.ambient_texname.length())
			tryAddingTexture(*mat, "ambient", directory + m.ambient_texname);

		if (m.specular_texname.length())
			tryAddingTexture(*mat, "specular", directory + m.specular_texname);

		if (m.alpha_texname.length())
			tryAddingTexture(*mat, "dissolve", directory + m.alpha_texname);

		m_material.push_back(std::move(mat));
	}

	std::cerr << "INF: creating shapes" << std::endl;
	// load shapes
	m_shapes.reserve(shapes.size());
	for (const auto& s : shapes)
	{
		int materialId = materials.size() - 1;
		if (s.mesh.material_ids.size() > 0)
		{
			materialId = s.mesh.material_ids[0];
			if(materialId < 0)
				materialId = materials.size() - 1;
		}


		m_shapes.push_back(std::make_unique<ObjShape>(
			VertexBuffer(s.mesh.indices, 1), 
			*m_material[materialId].get()));
	}
}

ObjModel::~ObjModel()
{
}

void ObjModel::prepareDrawing() const
{
	// just bind the vertex format
	assert(m_vao);
	m_vao->bind();
	m_vertices->bind(0);
	if (m_normals)
		m_normals->bind(1);
	if (m_texcoords)
		m_texcoords->bind(2);
}

const std::vector<std::unique_ptr<IShape>>& ObjModel::getShapes() const
{
	return m_shapes;
}

void ObjModel::tryAddingTexture(SimpleMaterial& material, const std::string& attrName, const std::string& textureName)
{
	try
	{
		auto tex = Texture2D::loadFromFile(textureName);
		material.addTexture(attrName, tex);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
