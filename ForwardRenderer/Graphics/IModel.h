#pragma once
#include <string>
#include "IShape.h"
#include <memory>
#include <vector>
#include "IMaterials.h"

class IModel
{
public:
	virtual ~IModel(){}
	// this function should be called before the shapes are rendered 
	// (this may for example set the vertex format that is only required once for all objects)
	virtual void prepareDrawing(IShader& shader) const = 0;
	virtual const std::vector<std::unique_ptr<IShape>>& getShapes() const = 0;
	virtual const IMaterials& getMaterial() const = 0;

	// functions for scene bounding box retrieval
	virtual const glm::vec3& getBoundingMin() const = 0;
	virtual const glm::vec3& getBoundingMax() const = 0;
};
