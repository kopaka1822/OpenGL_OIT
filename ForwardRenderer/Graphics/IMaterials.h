#pragma once
#include "../Framework/ParamSet.h"

/**
* \brief interface that should contain several materials and upload them to the gpu (as uniform block)
*/
class IMaterials
{
public:
	virtual ~IMaterials() = default;
	// reserve an amount of materials
	virtual void reserve(size_t count) = 0;
	// add a material to the local list
	virtual void addMaterial(ParamSet material) = 0;
	// upload the material to the gpu
	virtual void upload() = 0;
	// bind the material block
	virtual void bind(int materialId) const = 0;

	virtual const ParamSet& getMaterial(int materialId) const = 0;
};
