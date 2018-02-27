#pragma once
#include "Graphics/IModel.h"
#include <memory>
#include "Graphics/VertexArrayObject.h"
#include "Dependencies/gl/buffer.hpp"

class SimpleMaterial;

class ObjModel : public IModel
{
public:
	explicit ObjModel(const std::string& filename);
	~ObjModel();

	void prepareDrawing() const override;
	const std::vector<std::unique_ptr<IShape>>& getShapes() const override;

private:
	static void tryAddingTexture(SimpleMaterial& material, const std::string& attrName, const std::string& textureName);
private:
	gl::StaticShaderStorageBuffer m_vertices;
	gl::StaticShaderStorageBuffer m_normals;
	gl::StaticShaderStorageBuffer m_texcoords;
	std::unique_ptr<VertexArrayObject> m_vao;
	std::vector<std::unique_ptr<IShape>> m_shapes;
	std::vector<std::unique_ptr<SimpleMaterial>> m_material;
};

