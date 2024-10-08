#pragma once
#include "../Graphics/IModel.h"
#include <memory>
#include "../Dependencies/gl/buffer.hpp"
#include "../Graphics/SamplerCache.h"
#include "../Dependencies/gl/vertexarrayobject.hpp"
#include "../Framework/ParamSet.h"
#include "SimpleMaterial.h"

class SimpleMaterial;

class ObjModel : public IModel
{
public:
	explicit ObjModel(const std::string& filename);
	~ObjModel();

	void prepareDrawing(IShader& shader) const override;
	const std::vector<std::unique_ptr<IShape>>& getShapes() const override;

	const IMaterials& getMaterial() const override;


	const glm::vec3& getBoundingMin() const override
	{
		return m_bboxMin;
	}
	const glm::vec3& getBoundingMax() const override
	{
		return m_bboxMax;
	}
private:
	static void tryAddingTexture(ParamSet& material, const std::string& attrName, const std::string& textureName);
private:
	//gl::StaticShaderStorageBuffer m_vertices;
	//gl::StaticShaderStorageBuffer m_normals;
	//gl::StaticShaderStorageBuffer m_texcoords;
	gl::StaticShaderStorageBuffer m_vertices;
	gl::TextureBuffer m_verticesTextureView;

	gl::StaticShaderStorageBuffer m_normals;
	gl::TextureBuffer m_normalsTextureView;

	gl::StaticShaderStorageBuffer m_texcoords;
	gl::TextureBuffer m_texcoordsTextureView;

	gl::VertexArrayObject m_vao;
	std::vector<std::unique_ptr<IShape>> m_shapes;

	SimpleMaterial m_materials;

	glm::vec3 m_bboxMin;
	glm::vec3 m_bboxMax;
};

