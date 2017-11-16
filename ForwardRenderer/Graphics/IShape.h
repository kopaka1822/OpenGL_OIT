#pragma once

class IShader;

class IShape
{
public:
	virtual ~IShape(){}
	virtual void draw(IShader* shader) = 0;
	virtual bool isTransparent() const = 0;
};
