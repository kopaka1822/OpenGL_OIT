#pragma once
#include <string>
#include "IShape.h"
#include <memory>
#include <vector>

class IModel
{
public:
	virtual ~IModel(){}
	// this function should be called before the shapes are rendered 
	// (this may for example set the vertex format that is only required once for all objects)
	virtual void prepareDrawing(IShader& shader) const = 0;
	virtual const std::vector<std::unique_ptr<IShape>>& getShapes() const = 0;
};
