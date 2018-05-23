#include "ObjModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../Dependencies/tiny_obj_loader.h"
#include <iostream>
#include <chrono>
#include "ObjShape.h"
#include "../Graphics/SamplerCache.h"

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

	// make attribute buffer
	if (!attrib.vertices.size())
		throw std::runtime_error("no vertices found");

	m_vao.addAttribute(0, 0, gl::VertexType::INT32, 1, 0);
	m_vao.addAttribute(1, 0, gl::VertexType::INT32, 1, sizeof(int));
	m_vao.addAttribute(2, 0, gl::VertexType::INT32, 1, sizeof(int) * 2);

	m_vertices = gl::StaticShaderStorageBuffer(sizeof(attrib.vertices[0]), GLsizei(attrib.vertices.size()), attrib.vertices.data());
	m_verticesTextureView = gl::TextureBuffer(gl::TextureBufferFormat::RGB32F, m_vertices);

	if (!attrib.normals.empty())
	{
		m_normals = gl::StaticShaderStorageBuffer(sizeof(attrib.normals[0]), GLsizei(attrib.normals.size()), attrib.normals.data());
		m_normalsTextureView = gl::TextureBuffer(gl::TextureBufferFormat::RGB32F, m_normals);
	}
	if (!attrib.texcoords.empty())
	{
		m_texcoords = gl::StaticShaderStorageBuffer(sizeof(attrib.texcoords[0]), GLsizei(attrib.texcoords.size()), attrib.texcoords.data());
		m_texcoordsTextureView = gl::TextureBuffer(gl::TextureBufferFormat::RG32F, m_texcoords);
	}
	
	std::cerr << "INF: creating material" << std::endl;
	
	// Append `default` material
	materials.push_back(tinyobj::material_t());
	// load material
	m_materials.reserve(materials.size());
	for(const auto& m : materials)
	{
		auto mat = ParamSet();

		mat.add("name", m.name);

		mat.add("diffuse", glm::vec3(m.diffuse[0], m.diffuse[1], m.diffuse[2]));
		//mat->addAttribute("ambient", glm::vec4(m.ambient[0], m.ambient[1], m.ambient[2], 1.0f));
		mat.add("specular", glm::vec4(m.specular[0], m.specular[1], m.specular[2], m.shininess));
		mat.add("dissolve", m.dissolve);
		mat.add("illum", float(m.illum));
		mat.add("transmittance", glm::vec3(m.transmittance[0], m.transmittance[1], m.transmittance[2]));
		mat.get("refraction", m.ior);

		// add available attributes
		if (m.diffuse_texname.length())
			tryAddingTexture(mat, "diffuse", directory + m.diffuse_texname);

		if (m.ambient_texname.length())
			tryAddingTexture(mat, "ambient", directory + m.ambient_texname);

		if (m.specular_texname.length())
			tryAddingTexture(mat, "specular", directory + m.specular_texname);

		if (m.alpha_texname.length())
			tryAddingTexture(mat, "dissolve", directory + m.alpha_texname);

		m_materials.addMaterial(std::move(mat));
	}
	m_materials.upload();

	std::cerr << "INF: creating shapes" << std::endl;
	// load shapes
	m_shapes.reserve(shapes.size());
	for (const auto& s : shapes)
	{
		int materialId = int(materials.size()) - 1;
		if (!s.mesh.material_ids.empty())
		{
			materialId = s.mesh.material_ids[0];
			if(materialId < 0)
				materialId = int(materials.size()) - 1;
		}


		m_shapes.push_back(std::make_unique<ObjShape>(
			gl::StaticArrayBuffer(s.mesh.indices), *this, materialId));
	}
}

ObjModel::~ObjModel()
{
}

void ObjModel::prepareDrawing(IShader& shader) const
{
	// bind the vertex format
	m_vao.bind();
	m_verticesTextureView.bind(4);
	if (!m_normals.empty())
		m_normalsTextureView.bind(5);
	// workaround for texture usage stage warning
	else m_verticesTextureView.bind(5);

	if (!m_texcoords.empty())
		m_texcoordsTextureView.bind(6);
	// workaround for texture usage stage warning
	else m_verticesTextureView.bind(6);
}

const std::vector<std::unique_ptr<IShape>>& ObjModel::getShapes() const
{
	return m_shapes;
}

const IMaterials& ObjModel::getMaterial() const
{
	return m_materials;
}

void ObjModel::tryAddingTexture(ParamSet& material, const std::string& attrName, const std::string& textureName)
{
	try
	{
		auto tex = CachedTexture2D::loadFromFile(textureName);
		material.addTexture(attrName, tex);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
